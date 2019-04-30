#
# Jailhouse, a Linux-based partitioning hypervisor
#
# Copyright (c) OTH Regensburg, 2019
#
# Authors:
#  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.

from .extendedenum import ExtendedEnum


class PCI_CAP_ID(ExtendedEnum):
    _ids = {
        'PM'     : 0x01, # Power Management
        'VPD'    : 0x03, # Vital Product Data
        'MSI'    : 0x05, # Message Signalled Interrupts
        'HT'     : 0x08, # HyperTransport
        'VNDR'   : 0x09, # Vendor-Specific
        'DBG'    : 0x0A, # Debug port
        'SSVID'  : 0x0D, # Bridge subsystem vendor/device ID
        'SECDEV' : 0x0F, # Secure Device
        'EXP'    : 0x10, # PCI Express
        'MSIX'   : 0x11, # MSI-X
        'SATA'   : 0x12, # SATA Data/Index Conf.
        'AF'     : 0x13  # PCI Advanced Features
    }


class PCI_EXT_CAP_ID(ExtendedEnum):
    _ids = {
        'ERR'     : 0x01, # Advanced Error Reporting
        'VC'      : 0x02, # Virtual Channel Capability
        'DSN'     : 0x03, # Device Serial Number
        'PWR'     : 0x04, # Power Budgeting
        'RCLD'    : 0x05, # Root Complex Link Declaration
        'RCILC'   : 0x06, # Root Complex Internal Link Control
        'RCEC'    : 0x07, # Root Complex Event Collector
        'MFVC'    : 0x08, # Multi-Function VC Capability
        'VC9'     : 0x09, # same as _VC
        'RCRB'    : 0x0A, # Root Complex RB?
        'VNDR'    : 0x0B, # Vendor-Specific
        'CAC'     : 0x0C, # Config Access - obsolete
        'ACS'     : 0x0D, # Access Control Services
        'ARI'     : 0x0E, # Alternate Routing ID
        'ATS'     : 0x0F, # Address Translation Services
        'SRIOV'   : 0x10, # Single Root I/O Virtualization
        'MRIOV'   : 0x11, # Multi Root I/O Virtualization
        'MCAST'   : 0x12, # Multicast
        'PRI'     : 0x13, # Page Request Interface
        'AMD_XXX' : 0x14, # Reserved for AMD
        'REBAR'   : 0x15, # Resizable BAR
        'DPA'     : 0x16, # Dynamic Power Allocation
        'TPH'     : 0x17, # TPH Requester
        'LTR'     : 0x18, # Latency Tolerance Reporting
        'SECPCI'  : 0x19, # Secondary PCIe Capability
        'PMUX'    : 0x1A, # Protocol Multiplexing
        'PASID'   : 0x1B, # Process Address Space ID
        'DPC'     : 0x1D, # Downstream Port Containment
        'L1SS'    : 0x1E, # L1 PM Substates
        'PTM'     : 0x1F  # Precision Time Measurement
    }

    def __str__(self):
        return ExtendedEnum.__str__(self) + ' | JAILHOUSE_PCI_EXT_CAP'
