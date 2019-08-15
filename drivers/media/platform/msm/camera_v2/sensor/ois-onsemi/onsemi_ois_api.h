/**
 * @brief		LC898123F40 Global declaration & prototype declaration
 *
 * @author		Copyright (C) 2017, ON Semiconductor, all right reserved.
 *
 **/
 
#ifndef ONSEMI_OIS_API_H_
#define ONSEMI_OIS_API_H_

//==============================================================================
//
//==============================================================================
#define	MODULE_VENDOR	0
#define	MDL_VER			2
#define	SEMCO_VERNUM		0x01130111
#define	Primax_VERNUM		0x05130107

typedef	signed char			 INT_8;
typedef	short				 INT_16;
typedef	long                 INT_32;
typedef	long long            INT_64;
typedef	unsigned char       UINT_8;
typedef	unsigned short      UINT_16;
typedef	unsigned long       UINT_32;
typedef	unsigned long long	UINT_64;

//****************************************************
//	STRUCTURE DEFINE
//****************************************************
typedef struct {
	UINT_16				Index;
	const UINT_8*		MagicCode;
	UINT_16				SizeMagicCode;
	const UINT_8*		FromCode;
	UINT_16				SizeFromCode;
}	DOWNLOAD_TBL;

typedef struct STRECALIB {
	INT_16	SsFctryOffX ;
	INT_16	SsFctryOffY ;
	INT_16	SsRecalOffX ;
	INT_16	SsRecalOffY ;
	INT_16	SsDiffX ;
	INT_16	SsDiffY ;
} stReCalib ;

#define	WPB_OFF		0x01
#define   WPB_ON			0x00
#define	SUCCESS		0x00
#define	FAILURE			0x01

#define	USER_RESERVE			3
#define	ERASE_BLOCKS			(16 - USER_RESERVE)
#define   BURST_LENGTH                 ( 8*5 )
#define   DMB_COEFF_ADDRESS	0x21
#define   BLOCK_UNIT				0x200
#define   BLOCK_BYTE				2560
#define   SECTOR_SIZE			320
#define   HALF_SECTOR_ADD_UNIT	0x20
#define   FLASH_ACCESS_SIZE		32
#define	USER_AREA_START		(BLOCK_UNIT * ERASE_BLOCKS)

#define	CNT100MS				1352
#define	CNT200MS				2703

//==============================================================================
//
//==============================================================================
#define		F40_IO_ADR_ACCESS				0xC000
#define		F40_IO_DAT_ACCESS				0xD000
#define 	SYSDSP_DSPDIV					0xD00014
#define 	SYSDSP_SOFTRES					0xD0006C
#define 	OSCRSEL							0xD00090
#define 	OSCCURSEL						0xD00094
#define 	SYSDSP_REMAP					0xD000AC
#define 	SYSDSP_CVER						0xD00100
#define	 	FLASHROM_123F40					0xE07000
#define 	FLASHROM_F40_RDATL				(FLASHROM_123F40 + 0x00)
#define 	FLASHROM_F40_RDATH				(FLASHROM_123F40 + 0x04)
#define 	FLASHROM_F40_WDATL				(FLASHROM_123F40 + 0x08)
#define 	FLASHROM_F40_WDATH				(FLASHROM_123F40 + 0x0C)
#define 	FLASHROM_F40_ADR				(FLASHROM_123F40 + 0x10)
#define 	FLASHROM_F40_ACSCNT				(FLASHROM_123F40 + 0x14)
#define 	FLASHROM_F40_CMD				(FLASHROM_123F40 + 0x18)
#define 	FLASHROM_F40_WPB				(FLASHROM_123F40 + 0x1C)
#define 	FLASHROM_F40_INT				(FLASHROM_123F40 + 0x20)
#define 	FLASHROM_F40_RSTB_FLA			(FLASHROM_123F40 + 0x4CC)
#define 	FLASHROM_F40_UNLK_CODE1			(FLASHROM_123F40 + 0x554)
#define 	FLASHROM_F40_CLK_FLAON			(FLASHROM_123F40 + 0x664)
#define 	FLASHROM_F40_UNLK_CODE2			(FLASHROM_123F40 + 0xAA8)
#define 	FLASHROM_F40_UNLK_CODE3			(FLASHROM_123F40 + 0xCCC)

#define		READ_STATUS_INI					0x01000000



//==============================================================================
// Prototype
//==============================================================================
extern void        F40_IORead32A( struct msm_ois_ctrl_t *ctrl, uint32_t, uint32_t * ) ;
extern void        F40_IOWrite32A( struct msm_ois_ctrl_t *ctrl, uint32_t, uint32_t ) ;
extern uint8_t	   F40_ReadWPB( struct msm_ois_ctrl_t *ctrl ) ;
extern uint8_t	   F40_UnlockCodeSet( struct msm_ois_ctrl_t *ctrl ) ;
extern uint8_t	   F40_UnlockCodeClear(struct msm_ois_ctrl_t *ctrl) ;
extern uint8_t	   F40_FlashDownload( struct msm_ois_ctrl_t *ctrl, uint8_t, uint8_t, uint8_t ) ;
extern uint8_t	   F40_FlashUpdate( struct msm_ois_ctrl_t *ctrl, uint8_t, DOWNLOAD_TBL* ) ;
extern uint8_t	   F40_FlashBlockErase( struct msm_ois_ctrl_t *ctrl, uint32_t ) ;
extern uint8_t	   F40_FlashBurstWrite( struct msm_ois_ctrl_t *ctrl, const uint8_t *, uint32_t, uint32_t) ;
extern void        F40_FlashSectorRead( struct msm_ois_ctrl_t *ctrl, uint32_t, uint8_t * ) ;
extern void        F40_CalcChecksum( const uint8_t *, uint32_t, uint32_t *, uint32_t * ) ;
extern void        F40_CalcBlockChksum( struct msm_ois_ctrl_t *ctrl, uint8_t, uint32_t *, uint32_t * ) ;
extern void        F40_ReadCalData( struct msm_ois_ctrl_t *ctrl, uint32_t * , uint32_t *  ) ;
extern uint8_t    F40_GyroReCalib( struct msm_ois_ctrl_t *ctrl, stReCalib *  ) ;
extern uint8_t    F40_WrGyroOffsetData( struct msm_ois_ctrl_t *ctrl ) ;
extern uint8_t    F40_RdStatus( struct msm_ois_ctrl_t *ctrl, uint8_t) ;
extern uint8_t    F40_WriteCalData( struct msm_ois_ctrl_t *ctrl, uint32_t * , uint32_t *  );

#endif /* #ifndef ONSEMI_OIS_API_H_ */
