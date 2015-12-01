#pragma once

namespace AMDError
{
	enum
	{
		kAMDSuccess = 0,
		kAMDUndefinedError = 0xE8000001,
		kAMDBadHeaderError = 0xE8000002,
		kAMDNoResourcesError = 0xE8000003,
		kAMDReadError = 0xE8000004,
		kAMDWriteError = 0xE8000005,
		kAMDUnknownPacketError = 0xE8000006,
		kAMDInvalidArgumentError = 0xE8000007,
		kAMDNotFoundError = 0xE8000008,
		kAMDIsDirectoryError = 0xE8000009,
		kAMDPermissionError = 0xE800000A,
		kAMDNotConnectedError = 0xE800000B,
		kAMDTimeOutError = 0xE800000C,
		kAMDOverrunError = 0xE800000D,
		kAMDEOFError = 0xE800000E,
		kAMDUnsupportedError = 0xE800000F,
		kAMDFileExistsError = 0xE8000010,
		kAMDBusyError = 0xE8000011,
		kAMDCryptoError = 0xE8000012,
		kAMDInvalidResponseError = 0xE8000013,
		kAMDMissingKeyError = 0xE8000014,
		kAMDMissingValueError = 0xE8000015,
		kAMDGetProhibitedError = 0xE8000016,
		kAMDSetProhibitedError = 0xE8000017,
		kAMDRemoveProhibitedError = 0xE8000018,
		kAMDImmutableValueError = 0xE8000019,
		kAMDPasswordProtectedError = 0xE800001A,
		kAMDMissingHostIDError = 0xE800001B,
		kAMDInvalidHostIDError = 0xE800001C,
		kAMDSessionActiveError = 0xE800001D,
		kAMDSessionInactiveError = 0xE800001E,
		kAMDMissingSessionIDError = 0xE800001F,
		kAMDInvalidSessionIDError = 0xE8000020,
		kAMDMissingServiceError = 0xE8000021,
		kAMDInvalidServiceError = 0xE8000022,
		kAMDServiceLimitError = 0xE800005B,
		kAMDCheckinSetupFailedError = 0xE800005E,
		kAMDInvalidCheckinError = 0xE8000023,
		kAMDCheckinTimeoutError = 0xE8000024,
		kAMDCheckinConnectionFailedError = 0xE800005F,
		kAMDCheckinReceiveFailedError = 0xE8000060,
		kAMDCheckinResponseFailedError = 0xE8000061,
		kAMDCheckinOutOfMemoryError = 0xE8000069,
		kAMDCheckinSendFailedError = 0xE8000062,
		kAMDMissingPairRecordError = 0xE8000025,
		kAMDInvalidPairRecordError = 0xE800005C,
		kAMDSavePairRecordFailedError = 0xE8000068,
		kAMDInvalidActivationRecordError = 0xE8000026,
		kAMDMissingActivationRecordError = 0xE8000027,
		kAMDServiceProhibitedError = 0xE800005D,
		kAMDWrongDroidError = 0xE8000028,
		kAMDSUVerificationError = 0xE8000029,
		kAMDSUPatchError = 0xE800002A,
		kAMDSUFirmwareError = 0xE800002B,
		kAMDProvisioningProfileNotValid = 0xE800002C,
		kAMDSendMessageError = 0xE800002D,
		kAMDReceiveMessageError = 0xE800002E,
		kAMDMissingOptionsError = 0xE800002F,
		kAMDMissingImageTypeError = 0xE8000030,
		kAMDDigestFailedError = 0xE8000031,
		kAMDStartServiceError = 0xE8000032,
		kAMDInvalidDiskImageError = 0xE8000033,
		kAMDMissingDigestError = 0xE8000034,
		kAMDMuxError = 0xE8000035,
		kAMDApplicationAlreadyInstalledError = 0xE8000036,
		kAMDApplicationMoveFailedError = 0xE8000037,
		kAMDApplicationSINFCaptureFailedError = 0xE8000038,
		kAMDApplicationSandboxFailedError = 0xE8000039,
		kAMDApplicationVerificationFailedError = 0xE800003A,
		kAMDArchiveDestructionFailedError = 0xE800003B,
		kAMDBundleVerificationFailedError = 0xE800003C,
		kAMDCarrierBundleCopyFailedError = 0xE800003D,
		kAMDCarrierBundleDirectoryCreationFailedError = 0xE800003E,
		kAMDCarrierBundleMissingSupportedSIMsError = 0xE800003F,
		kAMDCommCenterNotificationFailedError = 0xE8000040,
		kAMDContainerCreationFailedError = 0xE8000041,
		kAMDContainerP0wnFailedError = 0xE8000042,
		kAMDContainerRemovalFailedError = 0xE8000043,
		kAMDEmbeddedProfileInstallFailedError = 0xE8000044,
		kAMDErrorError = 0xE8000045,
		kAMDExecutableTwiddleFailedError = 0xE8000046,
		kAMDExistenceCheckFailedError = 0xE8000047,
		kAMDInstallMapUpdateFailedError = 0xE8000048,
		kAMDManifestCaptureFailedError = 0xE8000049,
		kAMDMapGenerationFailedError = 0xE800004A,
		kAMDMissingBundleExecutableError = 0xE800004B,
		kAMDMissingBundleIdentifierError = 0xE800004C,
		kAMDMissingBundlePathError = 0xE800004D,
		kAMDMissingContainerError = 0xE800004E,
		kAMDNotificationFailedError = 0xE800004F,
		kAMDPackageExtractionFailedError = 0xE8000050,
		kAMDPackageInspectionFailedError = 0xE8000051,
		kAMDPackageMoveFailedError = 0xE8000052,
		kAMDPathConversionFailedError = 0xE8000053,
		kAMDRestoreContainerFailedError = 0xE8000054,
		kAMDSeatbeltProfileRemovalFailedError = 0xE8000055,
		kAMDStageCreationFailedError = 0xE8000056,
		kAMDSymlinkFailedError = 0xE8000057,
		kAMDiTunesArtworkCaptureFailedError = 0xE8000058,
		kAMDiTunesMetadataCaptureFailedError = 0xE8000059,
		kAMDAlreadyArchivedError = 0xE800005A,
		kAMDUnknownCommandError = 0xE8000066,
		kAMDAPIInternalError = 0xE8000067,
		kAMDMuxCreateListenerError = 0xE8000063,
		kAMDMuxGetListenerError = 0xE8000064,
		kAMDMuxConnectError = 0xE8000065,
		kAMDDeviceTooNewError = 0xE800006A,
		kAMDDeviceRefNoGood = 0xE800006B,
		kAMDCannotTranslateError = 0xE800006C,
		kAMDMobileImageMounterMissingImageSignature = 0xE800006D,
		kAMDMobileImageMounterResponseCreationFailed = 0xE800006E,
		kAMDMobileImageMounterMissingImageType = 0xE800006F,
		kAMDMobileImageMounterMissingImagePath = 0xE8000070,
		kAMDMobileImageMounterImageMapLoadFailed = 0xE8000071,
		kAMDMobileImageMounterAlreadyMounted = 0xE8000072,
		kAMDMobileImageMounterImageMoveFailed = 0xE8000073,
		kAMDMobileImageMounterMountPathMissing = 0xE8000074,
		kAMDMobileImageMounterMountPathNotEmpty = 0xE8000075,
		kAMDMobileImageMounterImageMountFailed = 0xE8000076,
		kAMDMobileImageMounterTrustCacheLoadFailed = 0xE8000077,
		kAMDMobileImageMounterDigestFailed = 0xE8000078,
		kAMDMobileImageMounterDigestCreationFailed = 0xE8000079,
		kAMDMobileImageMounterImageVerificationFailed = 0xE800007A,
		kAMDMobileImageMounterImageInfoCreationFailed = 0xE800007B,
		kAMDMobileImageMounterImageMapStoreFailed = 0xE800007C,
	};
};

/*
CFSTR("An undefined error occurred");
CFSTR("The operation header was invalid");
CFSTR("No resources are available for the requested operation");
CFSTR("A read error occurred");
CFSTR("A write error occurred");
CFSTR("Packet type unknown");
CFSTR("Invalid argument");
CFSTR("The requested object was not found");
CFSTR("The requested object is a directory");
CFSTR("Permission denied");
CFSTR("The service is not connected");
CFSTR("The requested operation timed out");
CFSTR("More data was received than requested");
CFSTR("End of data");
CFSTR("The requested operation is not supported");
CFSTR("The requested object already exists");
CFSTR("The requested object is busy");
CFSTR("No space is available");
CFSTR("The requested operation would block");
CFSTR("An error occurred during I/O processing");
CFSTR("The requested operation was interrupted");
CFSTR("The requested operation is already in progress");
CFSTR("An internal processing error occurred");
*/
namespace AFCInternalError
{
	t_uint32 ToExternalError(t_uint32 internal_error);
	enum 
	{
		kAFCSuccess = 0,
		kAFCUndefinedError = 0xE8004001,
		kAFCBadHeaderError = 0xE8004002,
		kAFCNoResourcesError = 0xE8004003,
		kAFCReadError = 0xE8004004,
		kAFCWriteError = 0xE8004005,
		kAFCUnknownPacketError = 0xE8004006,
		kAFCInvalidArgumentError = 0xE8004007,
		kAFCNotFoundError = 0xE8004008,
		kAFCIsDirectoryError = 0xE8004009,
		kAFCPermissionError = 0xE800400A,
		kAFCNotConnectedError = 0xE800400B,
		kAFCTimeOutError = 0xE800400C,
		kAFCOverrunError = 0xE800400D,
		kAFCEOFError = 0xE800400E,
		kAFCUnsupportedError = 0xE800400F,
		kAFCFileExistsError = 0xE8004010,
		kAFCBusyError = 0xE8004011,
		kAFCNoSpaceError = 0xE8004012,
		kAFCWouldBlockError = 0xE8004013,
		kAFCInputOutputError = 0xE8004014,
		kAFCInterruptedError = 0xE8004015,
		kAFCInProgressError = 0xE8004016,
		kAFCInternalError = 0xE8004017,
	};
}
