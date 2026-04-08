/*
 * rtl8125_hw.h — RTL8125 Hardware Definitions (excerpt)
 *
 * Chip profile enumerations, capability flags, register map,
 * descriptor flags, interrupt bits, PHY/MAC OCP addresses.
 *
 * These definitions are derived from public Realtek documentation
 * and the official Linux r8125 driver source.
 */

#ifndef RTL8125_HW_H
#define RTL8125_HW_H

/* ----------------------------------------------------------------
 * Chip Profile — auto-detected at runtime by reading TxConfig
 * ---------------------------------------------------------------- */

typedef enum _RTL8125_CHIP_PROFILE {
    RTL8125_CHIP_PROFILE_UNKNOWN = 0,
    RTL8125_CHIP_PROFILE_8125A   = 1,   /* Rev 00-02, 初代 2.5GbE */
    RTL8125_CHIP_PROFILE_8125B   = 2,   /* Rev 03-05, 主流版本 */
    RTL8125_CHIP_PROFILE_8125BP  = 3,   /* Rev 06-09, B 改良版 */
    RTL8125_CHIP_PROFILE_8125D   = 4,   /* Rev 0A-0D, 需 PHY firmware patch */
    RTL8125_CHIP_PROFILE_8125CP  = 5    /* Rev 0E+,   紧凑封装 */
} RTL8125_CHIP_PROFILE;

/* ----------------------------------------------------------------
 * PCI Identification
 * ---------------------------------------------------------------- */

#define RTL8125_VENDOR_ID                   0x10EC
#define RTL8125_DEVICE_ID                   0x8125
#define RTL8125_FAMILY_REVISION_ID_MIN      0x00
#define RTL8125_FAMILY_REVISION_ID_MAX      0x1F

/* TxConfig MAC version field — used to identify chip profile */
#define RTL8125_TXCFG_MAC_VERSION_MASK      0x7C800000UL
#define RTL8125_TXCFG_MAC_8125A             0x60800000UL
#define RTL8125_TXCFG_MAC_8125B             0x64000000UL
#define RTL8125_TXCFG_MAC_8125BP            0x68000000UL
#define RTL8125_TXCFG_MAC_8125D             0x68800000UL
#define RTL8125_TXCFG_MAC_8125CP            0x70800000UL

/* ----------------------------------------------------------------
 * Profile Capability & Quirk Flags
 * ---------------------------------------------------------------- */

#define RTL8125_PROFILE_CAPABILITY_FLAG_MSI                 0x00000001UL
#define RTL8125_PROFILE_CAPABILITY_FLAG_MSIX                0x00000002UL
#define RTL8125_PROFILE_CAPABILITY_FLAG_MULTI_QUEUE         0x00000004UL
#define RTL8125_PROFILE_CAPABILITY_FLAG_EXTENDED_L1OFF_CAP  0x00000008UL
#define RTL8125_PROFILE_CAPABILITY_FLAG_PHY_GIGA_LITE_TUNING 0x00000010UL
#define RTL8125_PROFILE_CAPABILITY_FLAG_ADVERTISE_2500      0x00000020UL

#define RTL8125_PROFILE_QUIRK_FLAG_TIMER_UNIT_DOUBLE        0x00000001UL
#define RTL8125_PROFILE_QUIRK_FLAG_PFM_PATCH_REQUIRED       0x00000002UL

/* ----------------------------------------------------------------
 * MSI-X Vector Counts (per chip profile)
 * ---------------------------------------------------------------- */

#define RTL8125_MAX_MSIX_VECTORS            32U
#define RTL8125_MAX_MSIX_VECTORS_8125A      4U
#define RTL8125_MAX_MSIX_VECTORS_8125B      32U
#define RTL8125_MAX_MSIX_VECTORS_8125BP     32U
#define RTL8125_MAX_MSIX_VECTORS_8125D      32U
#define RTL8125_MAX_MSIX_VECTORS_8125CP     32U

/* ----------------------------------------------------------------
 * TX / RX Descriptor Ring
 * ---------------------------------------------------------------- */

#define RTL8125_MAX_TX_DESC                 1024U
#define RTL8125_MAX_RX_DESC                 1024U
#define RTL8125_DEFAULT_TX_DESC             1024U
#define RTL8125_DEFAULT_RX_DESC             256U

/* Descriptor flag bits */
#define RTL8125_DESC_OWN                    0x80000000UL  /* HW owns this descriptor */
#define RTL8125_DESC_RING_END               0x40000000UL  /* Last descriptor in ring */
#define RTL8125_DESC_FIRST_FRAG             0x20000000UL  /* First fragment of packet */
#define RTL8125_DESC_LAST_FRAG              0x10000000UL  /* Last fragment of packet */

/* TX checksum offload bits (Opts2) */
#define RTL8125_DESC_TX_UDP_CSUM            0x80000000UL
#define RTL8125_DESC_TX_TCP_CSUM            0x40000000UL
#define RTL8125_DESC_TX_IP_CSUM             0x20000000UL
#define RTL8125_DESC_TX_IPV6_CSUM           0x10000000UL

/* TX Large Send Offload (LSO/GSO) bits */
#define RTL8125_DESC_TX_LSO_MSS_SHIFT       18U
#define RTL8125_DESC_TX_LSO_MSS_MAX         0x07FFU
#define RTL8125_DESC_TX_GSO_V4              0x04000000UL
#define RTL8125_DESC_TX_GSO_V6              0x02000000UL

/* ----------------------------------------------------------------
 * Register Map (MMIO offsets from BAR0)
 * ---------------------------------------------------------------- */

/* General */
#define RTL8125_REG_IDR0                    0x0000U  /* MAC address */
#define RTL8125_REG_MAR0                    0x0008U  /* Multicast filter */
#define RTL8125_REG_CHIP_CMD                0x0037U  /* Chip command */
#define RTL8125_REG_TX_CONFIG               0x0040U  /* TX configuration */
#define RTL8125_REG_RX_CONFIG               0x0044U  /* RX configuration */
#define RTL8125_REG_CFG9346                 0x0050U  /* 93C46/56 command */
#define RTL8125_REG_CONFIG1                 0x0052U
#define RTL8125_REG_CONFIG2                 0x0053U
#define RTL8125_REG_CONFIG3                 0x0054U
#define RTL8125_REG_CONFIG5                 0x0056U
#define RTL8125_REG_PHY_STATUS              0x006CU  /* PHY link status */
#define RTL8125_REG_CPLUS_CMD               0x00E0U  /* C+ command */

/* Descriptor ring base addresses */
#define RTL8125_REG_TX_DESC_START_ADDR_LOW  0x0020U
#define RTL8125_REG_TX_DESC_START_ADDR_HIGH 0x0024U
#define RTL8125_REG_RX_DESC_ADDR_LOW        0x00E4U
#define RTL8125_REG_RX_DESC_ADDR_HIGH       0x00E8U

/* Interrupt */
#define RTL8125_REG_IMR0_8125               0x0038U  /* Interrupt mask */
#define RTL8125_REG_ISR0_8125               0x003CU  /* Interrupt status */
#define RTL8125_REG_TX_POLL                 0x0090U  /* TX doorbell */

/* MAC / PHY OCP (On-Chip Peripheral) */
#define RTL8125_REG_MAC_OCP                 0x00B0U
#define RTL8125_REG_PHY_OCP                 0x00B8U

/* ----------------------------------------------------------------
 * Interrupt Bits
 * ---------------------------------------------------------------- */

#define RTL8125_INTR_RX_OK                  0x0001U
#define RTL8125_INTR_RX_ERR                 0x0002U
#define RTL8125_INTR_TX_OK                  0x0004U
#define RTL8125_INTR_TX_ERR                 0x0008U
#define RTL8125_INTR_RX_DESC_UNAVAIL        0x0010U
#define RTL8125_INTR_LINK_CHG               0x0020U
#define RTL8125_INTR_RX_FIFO_OVER           0x0040U
#define RTL8125_INTR_TX_DESC_UNAVAIL        0x0080U
#define RTL8125_INTR_SW_INT                 0x0100U
#define RTL8125_INTR_PCS_TIMEOUT            0x4000U
#define RTL8125_INTR_SYS_ERR                0x8000U

/* ----------------------------------------------------------------
 * Chip Command Bits
 * ---------------------------------------------------------------- */

#define RTL8125_CHIP_CMD_STOP_REQ           0x80U
#define RTL8125_CHIP_CMD_RESET              0x10U
#define RTL8125_CHIP_CMD_RX_ENABLE          0x08U
#define RTL8125_CHIP_CMD_TX_ENABLE          0x04U

/* ----------------------------------------------------------------
 * PHY OCP — Firmware Patch Interface
 * ---------------------------------------------------------------- */

#define RTL8125_PHY_OCP_PATCH_REQUEST       0xB820U
#define RTL8125_PHY_OCP_PATCH_STATE         0xB800U
#define RTL8125_PHY_OCP_PATCH_REQUEST_BIT   0x0010U
#define RTL8125_PHY_OCP_PATCH_READY_BIT     0x0040U
#define RTL8125_PHY_OCP_RAMCODE_ADDR        0xA436U
#define RTL8125_PHY_OCP_RAMCODE_DATA        0xA438U
#define RTL8125_PHY_RAMCODE_VERSION_ADDR    0x801EU

/* ----------------------------------------------------------------
 * PHY Link Speed Advertisement
 * ---------------------------------------------------------------- */

#define RTL8125_PHY_STATUS_2500F            0x00000400UL
#define RTL8125_PHY_STATUS_1000F            0x00000010UL
#define RTL8125_PHY_STATUS_100              0x00000008UL
#define RTL8125_PHY_STATUS_10               0x00000004UL
#define RTL8125_PHY_STATUS_LINK             0x00000002UL

#define RTL8125_ADVERTISE_10HALF            0x0020U
#define RTL8125_ADVERTISE_10FULL            0x0040U
#define RTL8125_ADVERTISE_100HALF           0x0080U
#define RTL8125_ADVERTISE_100FULL           0x0100U
#define RTL8125_ADVERTISE_1000FULL          0x0200U
#define RTL8125_ADVERTISE_2500FULL          0x0080U

#endif /* RTL8125_HW_H */
