#
# Jailhouse, a Linux-based partitioning hypervisor
#
# Copyright (c) Siemens AG, 2015-2020
#
# Authors:
#  Jan Kiszka <jan.kiszka@siemens.com>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.
#
# This script should help to create a basic jailhouse configuration file.
# It needs to be executed on the target machine, where it will gather
# information about the system. For more advanced scenarios you will have
# to change the generated C-code.

import struct

from .extendedenum import ExtendedEnum

# Keep the whole file in sync with include/jailhouse/cell-config.h.
_CONFIG_REVISION = 13


class CStruct:
    def _slots(self):
        attrs = []
        all_attr = [getattr(cls, '__slots__', ())
                    for cls in type(self).__mro__]
        for cls in all_attr:
            num = getattr(self, '_BIN_FIELD_NUM', 0)
            if len(cls) < num:
                continue

            for i in range(num):
                attrs += [cls[i]]

        return attrs

    @classmethod
    def parse(cls, data):
        obj, data = cls.parse_class(cls, data)
        return obj, data

    @staticmethod
    def parse_class(cls, data):
        obj = cls()
        slots = obj._slots()
        if len(slots) > 0:
            data_tuple = cls._BIN_FMT.unpack_from(data)
            for assigment in zip(slots, data_tuple):
                setattr(obj, *assigment)

        return obj, data[cls._BIN_FMT.size:]

    @staticmethod
    def parse_array(cls, num, data):
        array = []
        for i in range(num):
            obj, data = cls.parse(data)
            array += [obj]

        return array, data


def flag_str(enum_class, value, separator=' | '):
    flags = []
    while value:
        mask = 1 << (value.bit_length() - 1)
        flags.insert(0, str(enum_class(mask)))
        value &= ~mask
    return separator.join(flags)


class JAILHOUSE_MEM(ExtendedEnum, int):
    _ids = {
        'READ':         0x00001,
        'WRITE':        0x00002,
        'EXECUTE':      0x00004,
        'DMA':          0x00008,
        'IO':           0x00010,
        'COMM_REGION':  0x00020,
        'LOADABLE':     0x00040,
        'ROOTSHARED':   0x00080,
        'NO_HUGEPAGES': 0x00100,
        'IO_UNALIGNED': 0x08000,
        'IO_8':         0x10000,
        'IO_16':        0x20000,
        'IO_32':        0x40000,
        'IO_64':        0x80000,
    }


class MemRegion(CStruct):
    __slots__ = 'phys_start', 'virt_start', 'size', 'flags',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('QQQQ')

    def __init__(self):
        self.phys_start = 0
        self.virt_start = 0
        self.size = 0
        self.flags = 0

    def __str__(self):
        return ("  phys_start: 0x%016x\n" % self.phys_start) + \
               ("  virt_start: 0x%016x\n" % self.virt_start) + \
               ("  size:       0x%016x\n" % self.size) + \
               ("  flags:      " + flag_str(JAILHOUSE_MEM, self.flags))

    def is_ram(self):
        return ((self.flags & (JAILHOUSE_MEM.READ |
                               JAILHOUSE_MEM.WRITE |
                               JAILHOUSE_MEM.EXECUTE |
                               JAILHOUSE_MEM.DMA |
                               JAILHOUSE_MEM.IO |
                               JAILHOUSE_MEM.COMM_REGION |
                               JAILHOUSE_MEM.ROOTSHARED)) ==
                (JAILHOUSE_MEM.READ |
                 JAILHOUSE_MEM.WRITE |
                 JAILHOUSE_MEM.EXECUTE |
                 JAILHOUSE_MEM.DMA))

    def is_comm_region(self):
        return (self.flags & JAILHOUSE_MEM.COMM_REGION) != 0

    def phys_address_in_region(self, address):
        return address >= self.phys_start and \
            address < (self.phys_start + self.size)

    def phys_overlaps(self, region):
        if self.size == 0 or region.size == 0:
            return False
        return region.phys_address_in_region(self.phys_start) or \
            self.phys_address_in_region(region.phys_start)

    def virt_address_in_region(self, address):
        return address >= self.virt_start and \
            address < (self.virt_start + self.size)

    def virt_overlaps(self, region):
        if self.size == 0 or region.size == 0:
            return False
        return region.virt_address_in_region(self.virt_start) or \
            self.virt_address_in_region(region.virt_start)


class CacheRegion(CStruct):
    __slots__ = 'start', 'size', 'type', 'flags',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('IIBxH')


class Irqchip(CStruct):
    __slots__ = 'address', 'id', 'pin_base', 'pin_bitmap_lo', 'pin_bitmap_hi',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('QIIQQ')

    def __init__(self):
        self.address = 0
        self.id = 0
        self.pin_base = 0
        self.pin_bitmap_lo = 0
        self.pin_bitmap_hi = 0

    def is_standard(self):
        return self.address == 0xfec00000


class PIORegion(CStruct):
    __slots__ = 'base', 'length',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('HH')

    def __init__(self):
        self.base = 0
        self.length = 0


class CellConfig(CStruct):
    # slots with a '_' prefix in name are private
    __slots__ = 'name', '_flags', '_cpu_sets', \
                'memory_regions', 'cache_regions', 'irqchips', 'pio_regions', \
                '_pci_devices', '_pci_caps', '_stream_ids', \
                'vpci_irq_base', 'cpu_reset_address',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT_HDR = struct.Struct('=6sH')
    _BIN_FMT = struct.Struct('=32s4xIIIIIIIIIIQ8x32x')

    def __init__(self):
        self.name = ""
        self.memory_regions = []
        self.irqchips = []
        self.pio_regions = []
        self.vpci_irq_base = 0
        self.cpu_reset_address = 0

    @classmethod
    def parse(cls, data, root_cell=False):
        try:
            if not root_cell:
                (signature, revision) = cls._BIN_FMT_HDR.unpack_from(data)
                if signature != b'JHCELL':
                    raise RuntimeError('Not a cell configuration')
                if revision != _CONFIG_REVISION:
                    raise RuntimeError('Configuration file revision mismatch')
                data = data[cls._BIN_FMT_HDR.size:]

            self, data = cls.parse_class(cls, data)
            self.name = self.name.decode().strip('\0')
            data = data[self._cpu_sets:] # skip CPU set

            self.memory_regions, data = \
                cls.parse_array(MemRegion, self.memory_regions, data)
            self.cache_regions, data = \
                cls.parse_array(CacheRegion, self.cache_regions, data)
            self.irqchips, data = \
                cls.parse_array(Irqchip, self.irqchips, data)
            self.pio_regions, data = \
                cls.parse_array(PIORegion, self.pio_regions, data)

            return self
        except struct.error:
            raise RuntimeError('Not a %scell configuration' %
                               ('root ' if root_cell else ''))


class SystemConfig(CStruct):
    _BIN_FMT = struct.Struct('=4x')
    # ...followed by MemRegion as hypervisor memory
    _BIN_FMT_CONSOLE_AND_PLATFORM = struct.Struct('32x12x224x44x')

    # constructed fields
    __slots__ = 'hypervisor_memory', 'root_cell',

    def __init__(self):
        self.hypervisor_memory = MemRegion()
        self.root_cell = CellConfig()

    @classmethod
    def parse(cls, data):
        try:
            hdr_fmt = CellConfig._BIN_FMT_HDR
            (signature, revision) = hdr_fmt.unpack_from(data)
            if signature != b'JHSYST':
                raise RuntimeError('Not a root cell configuration')
            if revision != _CONFIG_REVISION:
                raise RuntimeError('Configuration file revision mismatch')

            self, data = cls.parse_class(cls, data[hdr_fmt.size:])
            self.hypervisor_memory, data = MemRegion.parse(data)

            offs = cls._BIN_FMT_CONSOLE_AND_PLATFORM.size
            offs += CellConfig._BIN_FMT_HDR.size # skip header inside rootcell
            self.root_cell = CellConfig.parse(data[offs:], root_cell=True)
            return self
        except struct.error:
            raise RuntimeError('Not a root cell configuration')
