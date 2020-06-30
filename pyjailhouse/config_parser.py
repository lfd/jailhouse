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
import copy
import io
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

    def save(self, stream):
        values = tuple()
        for field in self._slots():
            value = getattr(self, field, 0)
            values = values + (value,)

        stream.write(self._BIN_FMT.pack(*values))

    @staticmethod
    def save_array(itr, stream):
        for obj in itr:
            obj.save(stream)

    @classmethod
    def parse(cls, stream):
        obj = cls.parse_class(cls, stream)
        return obj

    @staticmethod
    def parse_class(cls, stream):
        fmt = cls._BIN_FMT
        data_tuple = fmt.unpack_from(stream.read(fmt.size))
        obj = cls()
        slots = obj._slots()
        if len(slots) > 0:
            for assigment in zip(slots, data_tuple):
                setattr(obj, *assigment)

        return obj

    @staticmethod
    def parse_array(cls, num, stream):
        array = []
        for i in range(num):
            obj = cls.parse(stream)
            array += [obj]

        return array


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
    __slots__ = 'address', 'id', 'pin_base',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('QII')
    _BIN_FMT_PIN_MAP = struct.Struct('4I')

    # constructed fields
    __slots__ += 'pin_bitmap',

    def __init__(self):
        self.address = 0
        self.id = 0
        self.pin_base = 0
        self.pin_bitmap = [0,0,0,0]

    def is_standard(self):
        return self.address == 0xfec00000

    def save(self, stream):
        super(self.__class__, self).save(stream)
        stream.write(self._BIN_FMT_PIN_MAP.pack(*self.pin_bitmap))

    @classmethod
    def parse(cls, stream):
        self = cls.parse_class(cls, stream)
        pin_fmt = cls._BIN_FMT_PIN_MAP
        self.pin_bitmap = pin_fmt.unpack_from(stream.read(pin_fmt.size))
        return self


class PIORegion(CStruct):
    __slots__ = 'base', 'length',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('HH')

    def __init__(self):
        self.base = 0
        self.length = 0


class PciDevice(CStruct):
    __slots__ = 'type', 'iommu', 'domain', 'bdf', 'bar_mask', '_caps_start', \
                '_caps_num', 'num_msi_vectors', 'msi_bits', \
                'num_msix_vectors', 'msix_region_size', 'msix_address', \
                'shmem_regions_start', 'shmem_dev_id', 'shmem_peers', \
                'shmem_protocol'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT_BAR_MASK = struct.Struct('6I')
    _BIN_FMT = struct.Struct('BBHH%usHHBBHHQIBBH' % _BIN_FMT_BAR_MASK.size)

    # constructed fields
    __slots__ += 'caps',

    TYPE_DEVICE = 1
    TYPE_BRIDGE = 2
    TYPE_IVSHMEM = 3

    def save(self, stream):
        temp = copy.copy(self)
        temp.bar_mask = self._BIN_FMT_BAR_MASK.pack(*self.bar_mask)
        temp._caps_num = len(self.caps)
        return super(self.__class__, temp).save(stream)

    @classmethod
    def parse(cls, stream):
        self = cls.parse_class(cls, stream)
        self.bar_mask = cls._BIN_FMT_BAR_MASK.unpack_from(self.bar_mask)
        return self


class PciCapability(CStruct):
    __slots__ = 'id', 'start', 'len', 'flags'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=HHHH')


class Console(CStruct):
    __slots__ = 'address', 'size', 'type', 'flags', 'divider', 'gate_nr', \
                'clock_reg'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=QIHHIIQ')


class CellConfig(CStruct):
    # slots with a '_' prefix in name are private
    __slots__ = 'name', 'flags', 'cpu_set', \
                'memory_regions', 'cache_regions', 'irqchips', 'pio_regions', \
                'pci_devices', '_pci_caps', 'stream_ids', \
                'vpci_irq_base', 'cpu_reset_address',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=32s4xIIIIIIIIIIQ8x')
    _BIN_FMT_HDR = struct.Struct('=6sH')
    _BIN_FMT_CPU = struct.Struct('Q')
    _BIN_FMT_STREAM_ID = struct.Struct('I')
    _BIN_SIGNATURE = b'JHCELL'

    # constructed fields
    __slots__ += 'console',

    def __init__(self):
        self.name = ""
        self.flags = 0
        self.cpu_set = set()
        self.memory_regions = []
        self.irqchips = []
        self.pio_regions = []
        self.pci_devices = []
        self.stream_ids = []
        self.vpci_irq_base = 0
        self.cpu_reset_address = 0
        self.console = Console()

    def save(self, stream, is_rootcell=False):
        # Flatten and deduplicate PCI capabilities
        pci_caps = []
        for idx,dev in enumerate(self.pci_devices):
            if not dev.caps:
                continue

            duplicate = False
            # look for duplicate capability patterns
            for dev_prev in self.pci_devices[:idx]:
                if dev_prev.caps == dev.caps:
                    dev._caps_start = dev_prev._caps_start
                    duplicate = True
                    break

            if not duplicate:
                dev._caps_start = len(pci_caps)
                pci_caps += dev.caps

        # Convert CPU set to bit array
        cpu_bits_cap = self._BIN_FMT_CPU.size * 8
        cpu_sets = [0]
        for cpu in self.cpu_set:
            if cpu >= cpu_bits_cap:
                cpu_sets += [0]
            cpu_sets[-1] |= 1 << (cpu % cpu_bits_cap)
        cpu_sets.reverse() #

        temp = copy.copy(self)
        temp.name = self.name.encode()
        temp.cpu_set = int(len(cpu_sets) * self._BIN_FMT_CPU.size)
        temp.memory_regions = len(self.memory_regions)
        temp.cache_regions = len(self.cache_regions)
        temp.irqchips = len(self.irqchips)
        temp.pio_regions = len(self.pio_regions)
        temp.pci_devices = len(self.pci_devices)
        temp.pci_caps = len(pci_caps)
        temp.stream_ids = len(self.stream_ids)

        if not is_rootcell:
            stream.write(self._BIN_FMT_HDR.pack(self._BIN_SIGNATURE, \
                                                _CONFIG_REVISION))

        super(self.__class__, temp).save(stream)
        self.console.save(stream)
        for cpu_set in cpu_sets:
            stream.write(self._BIN_FMT_CPU.pack(cpu_set))
        self.save_array(self.memory_regions, stream)
        self.save_array(self.cache_regions, stream)
        self.save_array(self.irqchips, stream)
        self.save_array(self.pio_regions, stream)
        self.save_array(self.pci_devices, stream)
        self.save_array(pci_caps, stream)
        for sid in self.stream_ids:
            stream.write(self._BIN_FMT_STREAM_ID.pack(sid))

    @classmethod
    def parse(cls, stream):
        self = cls.parse_class(cls, stream)
        self.name = self.name.decode().strip('\0')
        self.console = Console.parse(stream)

        cpu_fmt = cls._BIN_FMT_CPU
        cpu_set_num = int(self.cpu_set / cpu_fmt.size)
        self.cpu_set = set()
        for set_idx in range(cpu_set_num):
            cpu_bits = cpu_fmt.unpack_from(stream.read(cpu_fmt.size))
            for bit in range(cpu_fmt.size * 8):
                if cpu_bits[0] & (1 << bit) > 0:
                    self.cpu_set.add(bit)

        self.memory_regions = \
            cls.parse_array(MemRegion, self.memory_regions, stream)
        self.cache_regions = \
            cls.parse_array(CacheRegion, self.cache_regions, stream)
        self.irqchips = cls.parse_array(Irqchip, self.irqchips, stream)
        self.pio_regions = \
            cls.parse_array(PIORegion, self.pio_regions, stream)

        self.pci_devices = cls.parse_array(PciDevice, self.pci_devices, stream)
        pci_caps = cls.parse_array(PciCapability, self._pci_caps, stream)
        for pci_dev in self.pci_devices:
            start = pci_dev._caps_start
            end = start + pci_dev._caps_num
            pci_dev.caps = pci_caps[start:end]

        sid_fmt = cls._BIN_FMT_STREAM_ID
        sid_num = self.stream_ids
        self.stream_ids = []
        for i in range(sid_num):
            self.stream_ids += [sid_fmt.unpack_from(stream.read(sid_fmt.size))]

        return self


class IommuAmd(CStruct):
    __slots__ = 'bdf', 'base_cap', 'msi_cap', 'features'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=HBBI')


class IommuPvu(CStruct):
    __slots__ = 'tlb_base', 'tlb_size'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=QI')


class Iommu(CStruct):
    __slots__ = 'type', 'base', 'size',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=IQI')
    _BIN_PAD_SIZE = max(IommuAmd._BIN_FMT.size, IommuPvu._BIN_FMT.size)

    # constructed fields
    __slots__ += 'subtype',

    _MAX_UNITS = 8

    TYPE_AMD = 1
    TYPE_INTEL = 2
    TYPE_SMMUV3 = 3
    TYPE_PVU = 4

    def __init__(self, subtype = None):
        self.type = 0
        self.base = 0
        self.size = 0
        self.subtype = subtype

    def save(self, stream):
        super(self.__class__, self).save(stream)
        padding = self._BIN_PAD_SIZE
        if self.subtype:
            self.subtype.save(stream)
            padding -= self.subtype._BIN_FMT.size

        stream.write(bytes(padding))

    @classmethod
    def parse(cls, stream):
        self = cls.parse_class(cls, stream)
        sub_cls = None
        if self.type == cls.TYPE_AMD:
            sub_cls = IommuAmd
        elif self.type == cls.TYPE_PVU:
            sub_cls = IommuPvu

        if sub_cls:
            self.subtype = sub_cls.parse(stream)

        # skip rest of the C union, if needed
        skip = cls._BIN_PAD_SIZE - (sub_cls._BIN_FMT.size if sub_cls else 0)
        stream.seek(skip, io.SEEK_CUR)
        return self


class PlattformInfoArm(CStruct):
    __slots__ = 'maintenance_irq', 'gic_version', \
                'gicd_base', 'gicc_base', 'gich_base', 'gicv_base', 'gicr_base',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=BB2xQQQQQ')


class PlattformInfoX86(CStruct):
    __slots__ = 'pm_timer_address', 'vtd_interrupt_limit', 'apic_mode', \
                'tsc_khz', 'apic_khz',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=HIB1xII')


class PlattformInfo(CStruct):
    __slots__ = 'pci_mmconfig_base', 'pci_mmconfig_end_bus', \
                'pci_is_virtual', 'pci_domain',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=QBBH')
    _BIN_PAD_SIZE = max(PlattformInfoArm._BIN_FMT.size, \
                        PlattformInfoX86._BIN_FMT.size)

    # constructed fields
    __slots__ += 'iommus', 'arch_info',

    def __init__(self, arch_info_cls=PlattformInfoX86):
        self.pci_mmconfig_base = 0
        self.pci_mmconfig_end_bus = 0
        self.pci_is_virtual = 0
        self.pci_domain = 0
        self.iommus = []
        self.arch_info = arch_info_cls()

    def save(self, stream):
        super(self.__class__, self).save(stream)
        self.save_array(self.iommus, stream)
        self.arch_info.save(stream)
        padding = self._BIN_PAD_SIZE - self.arch_info._BIN_FMT.size
        stream.write(bytes(padding))

    @classmethod
    def parse(cls, stream, arch_info_cls=PlattformInfoX86):
        self = cls.parse_class(cls, stream)
        self.iommus = cls.parse_array(Iommu, Iommu._MAX_UNITS, stream)
        self.arch_info = arch_info_cls.parse(stream)

        # skip rest of the C union, if needed
        skip = cls._BIN_PAD_SIZE - arch_info_cls._BIN_FMT.size
        stream.seek(skip, io.SEEK_CUR)
        return self


class SystemConfig(CStruct):
    __slots__ = 'flags',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('I')
    _BIN_SIGNATURE = b'JHSYST'

    # constructed fields
    __slots__ += 'hypervisor_memory', 'debug_console', 'platform_info', \
                 'root_cell',

    def __init__(self):
        self.flags = 0
        self.hypervisor_memory = MemRegion()
        self.debug_console = Console()
        self.platform_info = PlattformInfo()
        self.root_cell = CellConfig()

    def save(self, stream):
        hdr_fmt = CellConfig._BIN_FMT_HDR
        stream.write(hdr_fmt.pack(self._BIN_SIGNATURE, _CONFIG_REVISION))
        super(self.__class__, self).save(stream)
        self.hypervisor_memory.save(stream)
        self.debug_console.save(stream)
        self.platform_info.save(stream)
        stream.write(bytes(hdr_fmt.size)) # place dummy cell header
        self.root_cell.save(stream, is_rootcell=True)

    @classmethod
    def parse(cls, stream):
        self = cls.parse_class(cls, stream)
        self.hypervisor_memory = MemRegion.parse(stream)
        self.debug_console = Console.parse(stream)
        self.platform_info = PlattformInfo.parse(stream)
        # skip header inside rootcell
        stream.seek(CellConfig._BIN_FMT_HDR.size, io.SEEK_CUR)
        self.root_cell = CellConfig.parse(stream)
        return self


def parse(stream, config_expect=None):
    fmt = CellConfig._BIN_FMT_HDR

    try:
        (signature, revision) = fmt.unpack_from(stream.read(fmt.size))
    except struct.error:
        raise RuntimeError('Not a Jailhouse configuration')

    if config_expect == None:
        # Try probing
        if signature == CellConfig._BIN_SIGNATURE:
            config_expect = CellConfig
        elif signature == SystemConfig._BIN_SIGNATURE:
            config_expect = SystemConfig
        else:
            raise RuntimeError('Not a Jailhouse configuration')
    elif config_expect._BIN_SIGNATURE != signature:
        raise RuntimeError("Not a '%s' configuration" % config_expect.__name__)

    if revision != _CONFIG_REVISION:
        raise RuntimeError('Configuration file revision mismatch')

    try:
        return config_expect.parse(stream)
    except struct.error:
        raise RuntimeError('Configuration unreadable')
