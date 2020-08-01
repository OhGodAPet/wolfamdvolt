#ifndef __VBIOS_TABLES_H
#define __VBIOS_TABLES_H

#include <stdint.h>

#define OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER		0x00000048L

typedef struct _ATOM_COMMON_TABLE_HEADER
{
	uint16_t usStructureSize;
	uint8_t  ucTableFormatRevision;   /*Change it when the Parser is not backward compatible */
	uint8_t  ucTableContentRevision;  /*Change it only when the table needs to change but the firmware */
							  /*Image can't be updated, while Driver needs to carry the new table! */
}ATOM_COMMON_TABLE_HEADER;

/****************************************************************************/	
// Structure stores the ROM header.
/****************************************************************************/	
typedef struct _ATOM_ROM_HEADER
{
	ATOM_COMMON_TABLE_HEADER		sHeader;
	uint8_t	 uaFirmWareSignature[4];    /*Signature to distinguish between Atombios and non-atombios, 
								  atombios should init it as "ATOM", don't change the position */
	uint16_t usBiosRuntimeSegmentAddress;
	uint16_t usProtectedModeInfoOffset;
	uint16_t usConfigFilenameOffset;
	uint16_t usCRC_BlockOffset;
	uint16_t usBIOS_BootupMessageOffset;
	uint16_t usInt10Offset;
	uint16_t usPciBusDevInitCode;
	uint16_t usIoBaseAddress;
	uint16_t usSubsystemVendorID;
	uint16_t usSubsystemID;
	uint16_t usPCI_InfoOffset; 
	uint16_t usMasterCommandTableOffset; /*Offset for SW to get all command table offsets, Don't change the position */
	uint16_t usMasterDataTableOffset;   /*Offset for SW to get all data table offsets, Don't change the position */
	uint8_t  ucExtendedFunctionCode;
	uint8_t  ucReserved;
}ATOM_ROM_HEADER;

typedef struct _ATOM_MASTER_LIST_OF_COMMAND_TABLES
{
	uint16_t ASIC_Init;                              //Function Table, used by various SW components,latest version 1.1
	uint16_t GetDisplaySurfaceSize;                  //Atomic Table,  Used by Bios when enabling HW ICON
	uint16_t ASIC_RegistersInit;                     //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
	uint16_t VRAM_BlockVenderDetection;              //Atomic Table,  used only by Bios
	uint16_t DIGxEncoderControl;										 //Only used by Bios
	uint16_t MemoryControllerInit;                   //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
	uint16_t EnableCRTCMemReq;                       //Function Table,directly used by various SW components,latest version 2.1
	uint16_t MemoryParamAdjust; 										 //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock if needed
	uint16_t DVOEncoderControl;                      //Function Table,directly used by various SW components,latest version 1.2
	uint16_t GPIOPinControl;												 //Atomic Table,  only used by Bios
	uint16_t SetEngineClock;                         //Function Table,directly used by various SW components,latest version 1.1
	uint16_t SetMemoryClock;                         //Function Table,directly used by various SW components,latest version 1.1
	uint16_t SetPixelClock;                          //Function Table,directly used by various SW components,latest version 1.2  
	uint16_t DynamicClockGating;                     //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
	uint16_t ResetMemoryDLL;                         //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
	uint16_t ResetMemoryDevice;                      //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
	uint16_t MemoryPLLInit;
	uint16_t AdjustDisplayPll;												//only used by Bios
	uint16_t AdjustMemoryController;                 //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock                
	uint16_t EnableASIC_StaticPwrMgt;                //Atomic Table,  only used by Bios
	uint16_t ASIC_StaticPwrMgtStatusChange;          //Obsolete ,     only used by Bios   
	uint16_t DAC_LoadDetection;                      //Atomic Table,  directly used by various SW components,latest version 1.2  
	uint16_t LVTMAEncoderControl;                    //Atomic Table,directly used by various SW components,latest version 1.3
	uint16_t LCD1OutputControl;                      //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t DAC1EncoderControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1  
	uint16_t DAC2EncoderControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t DVOOutputControl;                       //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t CV1OutputControl;                       //Atomic Table,  Atomic Table,  Obsolete from Ry6xx, use DAC2 Output instead 
	uint16_t GetConditionalGoldenSetting;            //only used by Bios
	uint16_t TVEncoderControl;                       //Function Table,directly used by various SW components,latest version 1.1
	uint16_t TMDSAEncoderControl;                    //Atomic Table,  directly used by various SW components,latest version 1.3
	uint16_t LVDSEncoderControl;                     //Atomic Table,  directly used by various SW components,latest version 1.3
	uint16_t TV1OutputControl;                       //Atomic Table,  Obsolete from Ry6xx, use DAC2 Output instead
	uint16_t EnableScaler;                           //Atomic Table,  used only by Bios
	uint16_t BlankCRTC;                              //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t EnableCRTC;                             //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t GetPixelClock;                          //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t EnableVGA_Render;                       //Function Table,directly used by various SW components,latest version 1.1
	uint16_t GetSCLKOverMCLKRatio;                   //Atomic Table,  only used by Bios
	uint16_t SetCRTC_Timing;                         //Atomic Table,  directly used by various SW components,latest version 1.1
	uint16_t SetCRTC_OverScan;                       //Atomic Table,  used by various SW components,latest version 1.1 
	uint16_t SetCRTC_Replication;                    //Atomic Table,  used only by Bios
	uint16_t SelectCRTC_Source;                      //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t EnableGraphSurfaces;                    //Atomic Table,  used only by Bios
	uint16_t UpdateCRTC_DoubleBufferRegisters;
	uint16_t LUT_AutoFill;                           //Atomic Table,  only used by Bios
	uint16_t EnableHW_IconCursor;                    //Atomic Table,  only used by Bios
	uint16_t GetMemoryClock;                         //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t GetEngineClock;                         //Atomic Table,  directly used by various SW components,latest version 1.1 
	uint16_t SetCRTC_UsingDTDTiming;                 //Atomic Table,  directly used by various SW components,latest version 1.1
	uint16_t ExternalEncoderControl;                 //Atomic Table,  directly used by various SW components,latest version 2.1
	uint16_t LVTMAOutputControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1
	uint16_t VRAM_BlockDetectionByStrap;             //Atomic Table,  used only by Bios
	uint16_t MemoryCleanUp;                          //Atomic Table,  only used by Bios    
	uint16_t ProcessI2cChannelTransaction;           //Function Table,only used by Bios
	uint16_t WriteOneByteToHWAssistedI2C;            //Function Table,indirectly used by various SW components 
	uint16_t ReadHWAssistedI2CStatus;                //Atomic Table,  indirectly used by various SW components
	uint16_t SpeedFanControl;                        //Function Table,indirectly used by various SW components,called from ASIC_Init
	uint16_t PowerConnectorDetection;                //Atomic Table,  directly used by various SW components,latest version 1.1
	uint16_t MC_Synchronization;                     //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
	uint16_t ComputeMemoryEnginePLL;                 //Atomic Table,  indirectly used by various SW components,called from SetMemory/EngineClock
	uint16_t MemoryRefreshConversion;                //Atomic Table,  indirectly used by various SW components,called from SetMemory or SetEngineClock
	uint16_t VRAM_GetCurrentInfoBlock;               //Atomic Table,  used only by Bios
	uint16_t DynamicMemorySettings;                  //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
	uint16_t MemoryTraining;                         //Atomic Table,  used only by Bios
	uint16_t EnableSpreadSpectrumOnPPLL;             //Atomic Table,  directly used by various SW components,latest version 1.2
	uint16_t TMDSAOutputControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1
	uint16_t SetVoltage;                             //Function Table,directly and/or indirectly used by various SW components,latest version 1.1
	uint16_t DAC1OutputControl;                      //Atomic Table,  directly used by various SW components,latest version 1.1
	uint16_t DAC2OutputControl;                      //Atomic Table,  directly used by various SW components,latest version 1.1
	uint16_t SetupHWAssistedI2CStatus;               //Function Table,only used by Bios, obsolete soon.Switch to use "ReadEDIDFromHWAssistedI2C"
	uint16_t ClockSource;                            //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
	uint16_t MemoryDeviceInit;                       //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
	uint16_t EnableYUV;                              //Atomic Table,  indirectly used by various SW components,called from EnableVGARender
	uint16_t DIG1EncoderControl;                     //Atomic Table,directly used by various SW components,latest version 1.1
	uint16_t DIG2EncoderControl;                     //Atomic Table,directly used by various SW components,latest version 1.1
	uint16_t DIG1TransmitterControl;                 //Atomic Table,directly used by various SW components,latest version 1.1
	uint16_t DIG2TransmitterControl;	               //Atomic Table,directly used by various SW components,latest version 1.1 
	uint16_t ProcessAuxChannelTransaction;					 //Function Table,only used by Bios
	uint16_t DPEncoderService;											 //Function Table,only used by Bios
	uint16_t GetVoltageInfo;
}ATOM_MASTER_LIST_OF_COMMAND_TABLES;   

typedef struct _ATOM_MASTER_LIST_OF_DATA_TABLES
{
	uint16_t        UtilityPipeLine;	        // Offest for the utility to get parser info,Don't change this position!
	uint16_t        MultimediaCapabilityInfo; // Only used by MM Lib,latest version 1.1, not configuable from Bios, need to include the table to build Bios 
	uint16_t        MultimediaConfigInfo;     // Only used by MM Lib,latest version 2.1, not configuable from Bios, need to include the table to build Bios
	uint16_t        StandardVESA_Timing;      // Only used by Bios
	uint16_t        FirmwareInfo;             // Shared by various SW components,latest version 1.4
	uint16_t        DAC_Info;                 // Will be obsolete from R600
	uint16_t        LCD_Info;                 // Shared by various SW components,latest version 1.3, was called LVDS_Info 
	uint16_t        TMDS_Info;                // Will be obsolete from R600
	uint16_t        AnalogTV_Info;            // Shared by various SW components,latest version 1.1 
	uint16_t        SupportedDevicesInfo;     // Will be obsolete from R600
	uint16_t        GPIO_I2C_Info;            // Shared by various SW components,latest version 1.2 will be used from R600           
	uint16_t        VRAM_UsageByFirmware;     // Shared by various SW components,latest version 1.3 will be used from R600
	uint16_t        GPIO_Pin_LUT;             // Shared by various SW components,latest version 1.1
	uint16_t        VESA_ToInternalModeLUT;   // Only used by Bios
	uint16_t        ComponentVideoInfo;       // Shared by various SW components,latest version 2.1 will be used from R600
	uint16_t        PowerPlayInfo;            // Shared by various SW components,latest version 2.1,new design from R600
	uint16_t        CompassionateData;        // Will be obsolete from R600
	uint16_t        SaveRestoreInfo;          // Only used by Bios
	uint16_t        PPLL_SS_Info;             // Shared by various SW components,latest version 1.2, used to call SS_Info, change to new name because of int ASIC SS info
	uint16_t        OemInfo;                  // Defined and used by external SW, should be obsolete soon
	uint16_t        XTMDS_Info;               // Will be obsolete from R600
	uint16_t        MclkSS_Info;              // Shared by various SW components,latest version 1.1, only enabled when ext SS chip is used
	uint16_t        Object_Header;            // Shared by various SW components,latest version 1.1
	uint16_t        IndirectIOAccess;         // Only used by Bios,this table position can't change at all!!
	uint16_t        MC_InitParameter;         // Only used by command table
	uint16_t        ASIC_VDDC_Info;						// Will be obsolete from R600
	uint16_t        ASIC_InternalSS_Info;			// New tabel name from R600, used to be called "ASIC_MVDDC_Info"
	uint16_t        TV_VideoMode;							// Only used by command table
	uint16_t        VRAM_Info;								// Only used by command table, latest version 1.3
	uint16_t        MemoryTrainingInfo;				// Used for VBIOS and Diag utility for memory training purpose since R600. the new table rev start from 2.1
	uint16_t        IntegratedSystemInfo;			// Shared by various SW components
	uint16_t        ASIC_ProfilingInfo;				// New table name from R600, used to be called "ASIC_VDDCI_Info" for pre-R600
	uint16_t        VoltageObjectInfo;				// Shared by various SW components, latest version 1.1
	uint16_t				PowerSourceInfo;					// Shared by various SW components, latest versoin 1.1
	uint16_t		ServiceInfo;
}ATOM_MASTER_LIST_OF_DATA_TABLES;

typedef struct _ATOM_MASTER_COMMAND_TABLE
{
	ATOM_COMMON_TABLE_HEADER           sHeader;
	ATOM_MASTER_LIST_OF_COMMAND_TABLES ListOfCommandTables;
}ATOM_MASTER_COMMAND_TABLE;

typedef struct _ATOM_MASTER_DATA_TABLE
{ 
	ATOM_COMMON_TABLE_HEADER sHeader;  
	ATOM_MASTER_LIST_OF_DATA_TABLES   ListOfDataTables;
}ATOM_MASTER_DATA_TABLE;

#endif
