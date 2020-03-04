#
# Jailhouse, a Linux-based partitioning hypervisor
#
# Copyright (c) Siemens AG, 2015-2019
# Copyright (c) OTH Regensburg, 2020
#
# Authors:
#  Jan Kiszka <jan.kiszka@siemens.com>
#  Andrej Utz <andrej.utz@st.oth-regensburg.de>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.

import copy
import struct

REVISION = 13


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

    def save(self):
        print("saving %s [%u B]" % (self.__class__.__name__, self._BIN_FMT.size))

        values = tuple()
        for field in self._slots():
            value = getattr(self, field, 0)
            #print("  ", field, " ", value)
            values = values + (value,)

        #print("Values: ", values)
        return self._BIN_FMT.pack(*values)

    @staticmethod
    def save_array(itr):
        if len(itr):
            print("saving array of %s [%u]" % (itr[0].__class__.__name__, len(itr)))

        data = b''
        for obj in itr:
            data += obj.save()

        return data

    @classmethod
    def parse(cls, data):
        obj, data = cls.parse_for_class(cls, data)
        return obj, data

    @staticmethod
    def parse_for_class(cls, data):
        obj = cls()
        slots = obj._slots()
        if len(slots) == 0:
            return None, data

        #print("parsing %s [%u]: %s" % (cls.__name__, cls._BIN_FMT.size, data[:cls._BIN_FMT.size]))
        print("parsing %s [%u B]: " % (cls.__name__, cls._BIN_FMT.size))
        data_tuple = cls._BIN_FMT.unpack_from(data)
        for assigment in zip(slots, data_tuple):
            #print("  ", *assigment)
            setattr(obj, *assigment)


        return obj, data[cls._BIN_FMT.size:]

    @staticmethod
    def parse_array(cls, num, data):
        _data = data
        if num:
            print("parsing array of %s [%u]" % (cls.__name__, num))

        lst = []
        for i in range(num):
            obj, data = cls.parse(data)
            lst += [obj]

        #if len(lst) > 0:
            #cls.data_cmp("ARRAY", _data, cls.save_array(lst))

        return lst, data


class CpuSet(CStruct):
    __slots__ = 'core_bits',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=Q')

    def __repr__(self):
        return 'CPU set: 0x%X' % (self.core_bits)


class MemRegion(CStruct):
    __slots__ = 'phys_start', 'virt_start', 'size', 'flags'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=QQQQ')

    FLAG_READ = 0x0001
    FLAG_WRITE = 0x0002
    FLAG_EXECUTE = 0x0004
    FLAG_DMA = 0x0008
    FLAG_IO = 0x0010
    FLAG_COMM_REGION = 0x0020
    FLAG_ROOTSHARED = 0x0080

    E820_RAM = 1
    E820_RESERVED = 2

    def __repr__(self):
        s = 'MemRegion: [0x%X-0x%X' % \
            (self.phys_start, self.phys_start + self.size)

        if self.virt_start > 0:
            s += ' -> 0x%X-0x%X' % \
                (self.virt_start, self.virt_start + self.size)

        s += ', flags: 0x%X]' % self.flags

        return s


class PortRegion(CStruct):
    __slots__ = 'base', 'length'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=HH')

    def __repr__(self):
        return 'Port I/O: [0x%X-0x%X]' % (self.base, self.base+self.length)


class CacheRegion(CStruct):
    __slots__ = 'start', 'size', 'type', 'flags'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=IIBxH')

    L3_CODE = 0x01
    L3_DATA = 0x02
    L3 = L3_CODE | L3_DATA
    ROOTSHARED = 0x0001

    def __repr__(self):
        return 'Cache: [0x%X-0x%X, type: 0x%X, flags: 0x%X]' % \
               (self.start, self.start+self.size, self.type, self.flags)


class Irqchip(CStruct):
    __slots__ = 'address', 'id', 'pin_base', 'pin_bitmap'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT_PIN_MAP = struct.Struct('4I')
    _BIN_FMT = struct.Struct('=QII%ss' % _BIN_FMT_PIN_MAP.size)

    def __repr__(self):
        return 'IRQChip: [0x%X, id: 0x%X]' % (self.address, self.id)

    def save(self):
        temp = copy.copy(self)
        temp.pin_bitmap = self._BIN_FMT_PIN_MAP.pack(*self.pin_bitmap)
        return super(self.__class__, temp).save()

    @classmethod
    def parse(cls, data):
        self, data = cls.parse_for_class(cls, data)
        self.pin_bitmap = cls._BIN_FMT_PIN_MAP.unpack_from(self.pin_bitmap)
        return self, data


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

    _MSI_BIT_64BITS = (1 << 0)
    _MSI_BIT_MASKABLE = (1 << 1)

    TYPE_DEVICE = 0x01
    TYPE_BRIDGE = 0x02
    TYPE_IVSHMEM = 0x03

    SHMEM_PROTO_UNDEFINED = 0x0000
    SHMEM_PROTO_VETH = 0x0001
    SHMEM_PROTO_CUSTOM = 0x4000  # 0x4000..0x7fff
    SHMEM_PROTO_VIRTIO_FRONT = 0x8000  # 0x8000..0xbfff
    SHMEM_PROTO_VIRTIO_BACK = 0xc000  # 0xc000..0xffff

    VIRTIO_DEV_NET = 1
    VIRTIO_DEV_BLOCK = 2
    VIRTIO_DEV_CONSOLE = 3

    IVSHMEM_BAR_MASK_INTX = [
        0xfffff000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000,
    ]

    IVSHMEM_BAR_MASK_MSIX = [
        0xfffff000, 0xfffffe00, 0x00000000,
        0x00000000, 0x00000000, 0x00000000,
    ]

    def __repr__(self, incl_class=True, incl_caps=True):
        s = '0x%X, ' % (self.bdf)
        s += 'iommu: 0x%X, ' % (self.iommu)
        s += 'msi vectors: %u' % self.num_msi_vectors

        if self.msi_bits > 0:
            f = []
            if self.msi_bits & self._MSI_BIT_64BITS:
                f += ['64bits']

            if self.msi_bits & self._MSI_BIT_MASKABLE:
                f += ['maskable']

            s += ', msi_flags %s' % f

        if incl_caps and getattr(self, 'caps', None) is not None:
            s += ', caps: %s' % self.caps

        if incl_class:
            s = '%s: [%s]' % (self.__class__.__name__, s)

        return s

    def save(self):
        temp = copy.copy(self)
        temp.bar_mask = self._BIN_FMT_BAR_MASK.pack(*self.bar_mask)
        temp._caps_num = len(self.caps)
        return super(self.__class__, temp).save()

    @classmethod
    def parse(cls, data):
        self, data = cls.parse_for_class(cls, data)
        self.bar_mask = cls._BIN_FMT_BAR_MASK.unpack_from(self.bar_mask)
        return self, data


class PciCapability(CStruct):
    __slots__ = 'id', 'start', 'len', 'flags'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=HHHH')

    def __repr__(self, incl_class=True):
        s = '0x%X-0x%X, id: 0x%X, flags: 0x%X' % \
            (self.id, self.start, self.start+self.len, self.flags)

        if incl_class:
            s = '%s: [%s]' % (self.__class__.__name__, s)

        return s

class StreamId(CStruct):
    __slots__ = 'id',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=I')


class Console(CStruct):
    __slots__ = 'address', 'size', 'type', 'flags', 'divider', 'gate_nr', \
                'clock_reg'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=QIHHIIQ')

    TYPE_NONE = 0x0000
    TYPE_EFIFB = 0x0001
    TYPE_8250 = 0x0002
    TYPE_PL011 = 0x0003
    TYPE_XUARTPS = 0x0004
    TYPE_MVEBU = 0x0005
    TYPE_HSCIF = 0x0006
    TYPE_SCIFA = 0x0007
    TYPE_IMX = 0x0008

    # Flags: bit 0 is used to select PIO (cleared) or MMIO (set) access
    FLAG_ACCESS_PIO = 0x0000
    FLAG_ACCESS_MMIO = 0x0001
    # Flags: bit 1 is used to select 1 (cleared) or 4-bytes (set) register
    #        distance.
    # 1 byte implied 8-bit access, 4 bytes 32-bit access.
    FLAG_REGDIST_1 = 0x0000
    FLAG_REGDIST_4 = 0x0002
    # Flags: bit 2 is used to select framebuffer format
    FLAG_FB_1024X768 = 0x0000
    FLAG_FB_1920X1080 = 0x0004
    # Bits 3-11: Reserved
    # Bit 12 is used to indicate to clear instead of to set the clock gate
    FLAG_INVERTED_GATE = 0x1000
    # Bits 13-15: Reserved

    def __repr__(self):
        return 'Console: [0x%X-0x%X, type: 0x%X, flags: 0x%X]' % \
            (self.address, self.address + self.size, self.type, self.flags)


class Cell(CStruct):
    __slots__ = 'name', 'id', 'flags', 'cpu_sets', \
                'memory_regions', 'cache_regions', 'irqchips', 'pio_regions', \
                'pci_devices', 'pci_caps', 'stream_ids', \
                'vpci_irq_base', 'cpu_reset_address', 'msg_reply_timeout'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=32sIIIIIIIIIIIQQ')
    _BIN_FMT_HDR = struct.Struct('=6sH')

    # constructed fields
    __slots__ += 'console',

    _SIGNATURE = b'JHCELL'

    def __repr__(self):
        s = 'name: %s, ' % self.name
        s += 'flags: 0x%X, ' % self.flags
        s += 'cpus: %s' % self.cpu_sets
        if len(self.memory_regions):
            s += ', mem: %s' % self.memory_regions

        if len(self.cache_regions):
            s += ', caches: %s' % self.cache_regions

        if len(self.pio_regions):
            s += ', ports: %s' % self.pio_regions

        if len(self.pci_devices):
            s += ', pci_devices: %s' % self.pci_devices

        s = 'Cell: [%s]' % s
        return s

    def save(self, is_rootcell=False):
        # Flatten and deduplicate PCI capabilities
        pci_caps = []
        for i,dev in enumerate(self.pci_devices):
            if not dev.caps:
                continue

            duplicate = False
            # look for duplicate capability patterns
            for dev_prev in self.pci_devices[:i]:
                if dev_prev.caps == dev.caps:
                    dev._caps_start = dev_prev._caps_start
                    duplicate = True
                    break

            if not duplicate:
                dev._caps_start = len(pci_caps)
                pci_caps += dev.caps

        temp = copy.copy(self)
        temp.name = self.name.encode()
        temp.cpu_sets = int(len(self.cpu_sets) * CpuSet._BIN_FMT.size)
        temp.memory_regions = len(self.memory_regions)
        temp.cache_regions = len(self.cache_regions)
        temp.irqchips = len(self.irqchips)
        temp.pio_regions = len(self.pio_regions)
        temp.pci_devices = len(self.pci_devices)
        temp.pci_caps = len(pci_caps)
        temp.stream_ids = len(self.stream_ids)

        data = bytes()
        if not is_rootcell:
            data += self._BIN_FMT_HDR.pack(self._SIGNATURE, REVISION)

        data += super(self.__class__, temp).save()
        data += self.console.save()
        data += self.save_array(self.cpu_sets)
        data += self.save_array(self.memory_regions)
        data += self.save_array(self.cache_regions)
        data += self.save_array(self.irqchips)
        data += self.save_array(self.pio_regions)
        data += self.save_array(self.pci_devices)
        data += self.save_array(pci_caps)
        data += self.save_array(self.stream_ids)

        return data

    @classmethod
    def parse(cls, data):
        self, data = cls.parse_for_class(cls, data)
        self.name = self.name.decode()
        self.console, data = Console.parse(data)

        cpu_set_num = int(self.cpu_sets / CpuSet._BIN_FMT.size)
        self.cpu_sets, data = \
            cls.parse_array(CpuSet, cpu_set_num, data)

        self.memory_regions, data = \
            cls.parse_array(MemRegion, self.memory_regions, data)

        self.cache_regions, data = \
            cls.parse_array(CacheRegion, self.cache_regions, data)

        self.irqchips, data = \
            cls.parse_array(Irqchip, self.irqchips, data)

        self.pio_regions, data = \
            cls.parse_array(PortRegion, self.pio_regions, data)

        self.pci_devices, data = \
            cls.parse_array(PciDevice, self.pci_devices, data)

        pci_caps, data = \
            cls.parse_array(PciCapability, self.pci_caps, data)

        for pci_dev in self.pci_devices:
            start = pci_dev._caps_start
            end = start + pci_dev._caps_num
            pci_dev.caps = pci_caps[start:end] # TODO: verify

        self.stream_ids, data = \
            cls.parse_array(StreamId, self.stream_ids, data)

        return self, data


class IommuAmd(CStruct):
    __slots__ = 'bdf', 'base_cap', 'msi_cap', 'features'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=HBBI')


class IommuPvu(CStruct):
    __slots__ = 'tlb_base', 'tlb_size'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=QI')


class Iommu(CStruct):
    __slots__ = 'type', 'base', 'size', 'subtype'
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=IQI%us' % \
        max(IommuAmd._BIN_FMT.size, IommuPvu._BIN_FMT.size))

    _MAX_UNITS = 8

    TYPE_AMD = 1
    TYPE_INTEL = 2
    TYPE_SMMUV3 = 3
    TYPE_PVU = 4

    def save(self):
        temp = copy.copy(self)
        temp.subtype = self.subtype.save() if self.subtype else bytes()

        #data += b'\x00' * (self.sizeof() - len(data)) # padding for union
        return super(self.__class__, temp).save()

    @classmethod
    def parse(cls, data):
        self, data = cls.parse_for_class(cls, data)

        if self.type == cls.TYPE_AMD:
            self.subtype, _ = IommuAmd.parse(self.subtype)
        elif self.type == cls.TYPE_PVU:
            self.subtype, _ = IommuPvu.parse(self.subtype)
        else:
            self.subtype = None

        return self, data



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
                'pci_is_virtual', 'pci_domain', 'iommus', 'arch_info',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=QBBH%us%us' % \
        (Iommu._BIN_FMT.size * Iommu._MAX_UNITS,
        max(PlattformInfoArm._BIN_FMT.size, PlattformInfoX86._BIN_FMT.size)))

    def save(self):
        temp = copy.copy(self)
        temp.iommus = self.save_array(self.iommus)
        temp.arch_info = self.arch_info.save()
        return super(self.__class__, temp).save()

    @classmethod
    def parse(cls, data, arch_info_cls=PlattformInfoX86):
        self, data = cls.parse_for_class(cls, data)
        self.iommus, _ = cls.parse_array(Iommu, Iommu._MAX_UNITS, self.iommus)
        self.arch_info, _ = arch_info_cls.parse(self.arch_info)
        return self, data


class System(CStruct):
    __slots__ = 'flags',
    _BIN_FIELD_NUM = len(__slots__)
    _BIN_FMT = struct.Struct('=I')

    # constructed fields
    __slots__ += 'hypervisor_memory', 'rootcell',

    _SIGNATURE = b'JHSYST'

    def __repr__(self):
        s = 'System: [name: %s, ' % self.rootcell.name
        s += self.rootcell.__repr__()
        s += ']'
        return s

    def save(self):
        data = Cell._BIN_FMT_HDR.pack(self._SIGNATURE, REVISION)
        data += super(self.__class__, self).save()
        data += self.hypervisor_memory.save()
        data += self.debug_console.save()
        data += self.platform_info.save()
        data += Cell._BIN_FMT_HDR.pack(bytes(), 0) # place dummy header
        data += self.rootcell.save(is_rootcell=True)
        return data

    @classmethod
    def parse(cls, data):
        self, data = cls.parse_for_class(cls, data)
        self.hypervisor_memory, data = MemRegion.parse(data)
        self.debug_console, data = Console.parse(data)
        self.platform_info, data = PlattformInfo.parse(data)
        data = data[Cell._BIN_FMT_HDR.size:] # skip header inside rootcell
        self.rootcell, data = Cell.parse(data)
        return self, data


def parse_config(data):
    fmt = Cell._BIN_FMT_HDR
    (signature, revision) = fmt.unpack_from(data)

    if signature == Cell._SIGNATURE:
        cell_type = Cell
    elif signature == System._SIGNATURE:
        cell_type = System
    else:
        raise RuntimeError('Not a cell configuration')

    if revision != REVISION:
        raise RuntimeError('Configuration file revision mismatch')

    return cell_type.parse(data[fmt.size:])


##### Testsandbox

config_data = open('configs/x86/ryzen.cell', 'rb').read()
config, _ = parse_config(config_data)
print(config)
config_data_new = config.save()

config, _ = parse_config(config_data_new)
print(config)
config_data_new2 = config.save()


def data_cmp(desc, data_r, data_s, full=False):
    data_o = data_r if full else data_r[:len(data_s)]

    if data_s == data_o:
        print("cmp: %s OK" % desc)
    else:
        print("cmp: %s FAIL" % desc)
        #print('  orig[%u]: ' % len(data_o), data_o)
        #print('  save[%u]: ' % len(data_s), data_s)

    return data_o


data_cmp("full config", config_data, config_data_new, True)

