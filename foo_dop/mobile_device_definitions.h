#pragma once

#define __attribute__(x)

/* Error codes */
#define MDERR_APPLE_MOBILE  (err_system(0x3a))
#define MDERR_IPHONE        (err_sub(0))

/* Apple Mobile (AM*) errors */
#define MDERR_OK                ERR_SUCCESS
#define MDERR_SYSCALL           (ERR_MOBILE_DEVICE | 0x01)
#define MDERR_OUT_OF_MEMORY     (ERR_MOBILE_DEVICE | 0x03)
#define MDERR_QUERY_FAILED      (ERR_MOBILE_DEVICE | 0x04)
#define MDERR_INVALID_ARGUMENT  (ERR_MOBILE_DEVICE | 0x0b)
#define MDERR_DICT_NOT_LOADED   (ERR_MOBILE_DEVICE | 0x25)

/* Apple File Connection (AFC*) errors */
#define MDERR_AFC_OUT_OF_MEMORY 0x03

/* USBMux errors */
#define MDERR_USBMUX_ARG_NULL   0x16
#define MDERR_USBMUX_FAILED     0xffffffff

/* Messages passed to device notification callbacks: passed as part of
* am_device_notification_callback_info. */
#define ADNCI_MSG_CONNECTED     1
#define ADNCI_MSG_DISCONNECTED  2
#define ADNCI_MSG_UNKNOWN       3

/* Services, found in /System/Library/Lockdown/Services.plist */
#define AMSVC_AFC                   ("com.apple.afc")
#define AMSVC_BACKUP                ("com.apple.mobilebackup")
#define AMSVC_CRASH_REPORT_COPY     ("com.apple.crashreportcopy")
#define AMSVC_DEBUG_IMAGE_MOUNT     ("com.apple.mobile.debug_image_mount")
#define AMSVC_NOTIFICATION_PROXY    ("com.apple.mobile.notification_proxy")
#define AMSVC_PURPLE_TEST           ("com.apple.purpletestr")
#define AMSVC_SOFTWARE_UPDATE       ("com.apple.mobile.software_update")
#define AMSVC_SYNC                  ("com.apple.mobilesync")
#define AMSVC_SCREENSHOT            ("com.apple.screenshotr")
#define AMSVC_SYSLOG_RELAY          ("com.apple.syslog_relay")
#define AMSVC_SYSTEM_PROFILER       ("com.apple.mobile.system_profiler")

typedef unsigned int afc_error_t;
typedef unsigned int usbmux_error_t;

struct am_recovery_device;

struct am_device_notification_callback_info {
   struct am_device *dev;  /* 0    device */
   unsigned int msg;       /* 4    one of ADNCI_MSG_* */
   struct am_device_notification * data;
} __attribute__ ((packed));

/* The type of the device restore notification callback functions.
* TODO: change to correct type. */
typedef void (*am_restore_device_notification_callback)(struct
   am_recovery_device *);

/* This is a CoreFoundation object of class AMRecoveryModeDevice. */
struct am_recovery_device {
       void *unknown0;
       void *unknown1;
   am_restore_device_notification_callback callback;   /* 8 */
   void *user_info;                                    /* 12 */
   unsigned int unknown3[1];                         /* 16 */
   unsigned int readwrite_pipe;                        /* 28 */
   unsigned int read_pipe;                            /* 32 */
   unsigned int write_ctrl_pipe;                      /* 33 */
   unsigned int read_unknown_pipe;                    /* 34 */
   unsigned int write_file_pipe;                      /* 35 */
   unsigned int write_input_pipe;                     /* 36 */
} __attribute__ ((packed));

/* A CoreFoundation object of class AMRestoreModeDevice. */
struct am_restore_device {
   unsigned char unknown[32];
   int port;
} __attribute__ ((packed));

/* The type of the device notification callback function. */
typedef void(*am_device_notification_callback)(struct
   am_device_notification_callback_info *, void *);

/* The type of the _AMDDeviceAttached function.
* TODO: change to correct type. */
typedef void *amd_device_attached_callback;

/* The type of the device restore notification callback functions.
* TODO: change to correct type. */
typedef void (*am_restore_device_notification_callback)(struct
   am_recovery_device *);

#if 1
struct am_device {void * dummy;};
#else
struct am_device {
   unsigned char unknown0[16]; /* 0 - zero */
   unsigned int device_id;     /* 16 */
   unsigned int product_id;    /* 20 - set to AMD_IPHONE_PRODUCT_ID */
   char *serial;               /* 24 - set to AMD_IPHONE_SERIAL */
   unsigned int unknown1;      /* 28 */ //refcount
   unsigned char unknown2[4];  /* 32 */
   unsigned int lockdown_conn; /* 36 */
   unsigned char unknown3[8];  /* 40 */
} __attribute__ ((packed));
#endif

struct am_device_notification {
   unsigned int unknown0;                      /* 0 */
   unsigned int unknown1;                      /* 4 */
   unsigned int unknown2;                      /* 8 */
   am_device_notification_callback callback;   /* 12 */
   unsigned int unknown3;                      /* 16 */
} __attribute__ ((packed));

struct afc_connection {
   unsigned int handle;            /* 0 */
   unsigned int unknown0;          /* 4 */
   unsigned char unknown1;         /* 8 */
   unsigned char padding[3];       /* 9 */
   unsigned int unknown2;          /* 12 */
   unsigned int unknown3;          /* 16 */
   unsigned int unknown4;          /* 20 */
   unsigned int fs_block_size;     /* 24 */
   unsigned int sock_block_size;   /* 28: always 0x3c */
   unsigned int io_timeout;        /* 32: from AFCConnectionOpen, usu. 0 */
   void *afc_lock;                 /* 36 */
   unsigned int context;           /* 40 */
} __attribute__ ((packed));

struct afc_device_info {
   unsigned char unknown[12];  /* 0 */
} __attribute__ ((packed));

struct afc_directory {
   unsigned char unknown[1];   /* size unknown */
} __attribute__ ((packed));

struct afc_dictionary {
   unsigned char unknown[1];   /* size unknown */
} __attribute__ ((packed));

typedef unsigned long long afc_file_ref;

struct usbmux_listener_1 {                  /* offset   value in iTunes */
   unsigned int unknown0;                  /* 0        1 */
   unsigned char *unknown1;                /* 4        ptr, maybe device? */
   amd_device_attached_callback callback;  /* 8        _AMDDeviceAttached */
   unsigned int unknown3;                  /* 12 */
   unsigned int unknown4;                  /* 16 */
   unsigned int unknown5;                  /* 20 */
} __attribute__ ((packed));

struct usbmux_listener_2 {
   unsigned char unknown0[4144];
} __attribute__ ((packed));

struct am_bootloader_control_packet {
   unsigned char opcode;       /* 0 */
   unsigned char length;       /* 1 */
   unsigned char magic[2];     /* 2: 0x34, 0x12 */
   unsigned char payload[1];   /* 4 */ //unknown size
} __attribute__ ((packed));

typedef const CFBooleanRef * p_kCFBooleanTrue_t;
typedef const CFBooleanRef * p_kCFBooleanFalse_t;

enum 
{
	afc_file_mode_read = 1,
	afc_file_mode_read_write = 2,
	afc_file_mode_write_new = 3,
	afc_file_mode_read_write_new = 4,
	afc_file_mode_write = 5,
	afc_file_mode_write_read = 6, //difference to afc_file_mode_read_write?
};

typedef CFStringRef  (__cdecl * p___CFStringMakeConstantString_t)(const char *cStr);
typedef CFIndex (__cdecl * p_CFStringGetLength_t)(CFStringRef theString);
typedef void (__cdecl * p_CFStringGetCharacters_t)(CFStringRef theString, CFRange range, UniChar *buffer);
typedef void (__cdecl * p_CFRelease_t)(CFTypeRef cf);
typedef CFTypeID (__cdecl * p_CFDictionaryGetTypeID_t)(void);
typedef void (__cdecl * p_CFDictionaryGetKeysAndValues_t)(CFDictionaryRef theDict, const void **keys, const void **values);
typedef CFIndex (__cdecl * p_CFDictionaryGetCount_t)(CFDictionaryRef theDict);
typedef CFTypeID (__cdecl * p_CFStringGetTypeID_t)(void);
typedef CFTypeID (__cdecl * p_CFBooleanGetTypeID_t)(void);
typedef CFTypeID (__cdecl * p_CFNumberGetTypeID_t)(void);
typedef CFNumberRef (__cdecl * p_CFNumberCreate_t)(CFAllocatorRef allocator, CFNumberType theType, const void *valuePtr);
typedef Boolean (__cdecl * p_CFBooleanGetValue_t)(CFBooleanRef boolean);
typedef Boolean (__cdecl * p_CFNumberGetValue_t)(CFNumberRef number, CFNumberType theType, void *valuePtr);
typedef CFTypeID (__cdecl * p_CFGetTypeID_t)(CFTypeRef cf);
typedef Boolean (__cdecl * p_CFNumberIsFloatType_t)(CFNumberRef number);
typedef CFTypeID (__cdecl * p_CFArrayGetTypeID_t)(void);
typedef CFIndex (__cdecl * p_CFArrayGetCount_t)(CFArrayRef theArray);
typedef const void *(__cdecl * p_CFArrayGetValueAtIndex_t)(CFArrayRef theArray, CFIndex idx);
typedef CFTypeRef (__cdecl * p_AMDeviceCopyValue_t)(struct am_device *device, const __CFString * pclass, const __CFString *pvalue);
typedef mach_error_t (__cdecl * p_AMDeviceSetValue_t)(struct am_device *device, const __CFString * pclass, const __CFString *pparam, const CFTypeRef pvalue);
typedef CFTypeID (__cdecl * p_CFDataGetTypeID_t)(void);
typedef CFIndex (__cdecl * p_CFDataGetLength_t)(CFDataRef theData);
typedef void (__cdecl * p_CFDataGetBytes_t)(CFDataRef theData, CFRange range, UInt8 *buffer); 
typedef mach_error_t (__cdecl * p_AMDeviceNotificationSubscribe_t)(am_device_notification_callback callback, unsigned int unused0, unsigned int unused1, unsigned int dn_unknown3, struct am_device_notification **notification);
typedef mach_error_t (__cdecl * p_AMDeviceConnect_t)(struct am_device *device);
typedef int (__cdecl * p_AMDeviceIsPaired_t)(struct am_device *device);
typedef mach_error_t (__cdecl * p_AMDeviceValidatePairing_t)(struct am_device *device);
typedef mach_error_t (__cdecl * p_AMDevicePair_t)(struct am_device *device);
typedef mach_error_t (__cdecl * p_AMDeviceStartSession_t)(struct am_device *device);
typedef mach_error_t (__cdecl * p_AMDeviceStartService_t)(struct am_device *device, CFStringRef service_name, SOCKET *handle, unsigned int * unknown);
 typedef mach_error_t (__cdecl * p_AMDeviceSecureStartService_t)(struct am_device *device, CFStringRef service_name, CFDictionaryRef options, SOCKET *handle);
typedef mach_error_t (__cdecl * p_AMDeviceStopSession_t)(struct am_device *device);
typedef mach_error_t (__cdecl * p_AMDeviceDisconnect_t)(struct am_device *device);
typedef struct am_device * (__cdecl * p_AMDeviceRetain_t)(struct am_device *device);
typedef void (__cdecl * p_AMDeviceRelease_t)(struct am_device *device);
typedef mach_error_t (__cdecl * p_AMDeviceNotificationUnsubscribe_t)(struct am_device_notification *notification);
//typedef HANDLE (__cdecl * p_AMDeviceNotificationGetThreadHandle_t)(struct am_device_notification *notification);
typedef afc_error_t (__cdecl * p_AFCConnectionOpen_t)(SOCKET handle, unsigned int io_timeout, struct afc_connection **conn);
typedef afc_error_t (__cdecl * p_AFCDeviceInfoOpen_t)(struct afc_connection *conn, struct afc_dictionary **info);
typedef afc_error_t (__cdecl * p_AFCDirectoryOpen_t)(struct afc_connection *conn, const char *path, struct afc_directory **dir);
typedef afc_error_t (__cdecl * p_AFCDirectoryRead_t)(struct afc_connection *conn/*unsigned int unused*/, struct afc_directory *dir, char **dirent);
typedef afc_error_t (__cdecl * p_AFCDirectoryClose_t)(afc_connection *conn, struct afc_directory *dir);
typedef afc_error_t (__cdecl * p_AFCDirectoryCreate_t)(afc_connection *conn, const char *dirname);
typedef afc_error_t (__cdecl * p_AFCRemovePath_t)(afc_connection *conn, const char *dirname);
typedef afc_error_t (__cdecl * p_AFCRenamePath_t)(afc_connection *conn, const char *oldpath, const char *newpath);
typedef afc_error_t (__cdecl * p_AFCConnectionClose_t)(struct afc_connection *conn);
typedef void (__cdecl * p_AFCConnectionSetIOTimeout_t)(struct afc_connection *conn, long secs, long microsecs);
typedef afc_error_t (__cdecl * p_AFCFileRefOpen_t)(struct afc_connection *conn, const char *path, unsigned long long int mode, afc_file_ref *ref);
typedef afc_error_t (__cdecl * p_AFCFileRefRead_t)(struct afc_connection *conn, afc_file_ref ref, void *buf, unsigned int *len);
typedef afc_error_t (__cdecl * p_AFCFileRefWrite_t)(struct afc_connection *conn, afc_file_ref ref, const void *buf, unsigned int len);
typedef afc_error_t (__cdecl * p_AFCFileRefClose_t)(struct afc_connection *conn, afc_file_ref ref);
typedef afc_error_t (__cdecl * p_AFCFileRefLock_t)(struct afc_connection *conn, afc_file_ref ref, bool unk2);
typedef afc_error_t (__cdecl * p_AFCFileRefUnlock_t)(struct afc_connection *conn, afc_file_ref ref);
typedef afc_error_t (__cdecl * p_AFCFileInfoOpen_t)(struct afc_connection *conn, const char *path, struct afc_dictionary **info);
typedef afc_error_t (__cdecl * p_AFCKeyValueRead_t)(struct afc_dictionary *dict, char **key, char ** val);
typedef afc_error_t (__cdecl * p_AFCKeyValueClose_t)(struct afc_dictionary *dict);
typedef afc_error_t (__cdecl * p_AFCFileRefSeek_t)(struct afc_connection *conn, afc_file_ref ref, unsigned long long offset1, unsigned long long offset2);
typedef afc_error_t (__cdecl * p_AFCFileRefSetFileSize_t)(struct afc_connection *conn, afc_file_ref ref, unsigned long long offset);
typedef afc_error_t (__cdecl * p_AFCFileRefTell_t)(struct afc_connection *conn, afc_file_ref ref, t_int64 * position);
typedef mach_error_t (__cdecl * p_AMDPostNotification_t)(SOCKET handle, CFStringRef name, CFStringRef userinfo);
typedef mach_error_t (__cdecl * p_AMDShutdownNotificationProxy_t)(SOCKET handle);
typedef mach_error_t (__cdecl * p_AMSInitialize_t)();
typedef mach_error_t (__cdecl * p_AMSCleanup_t)();
typedef mach_error_t (__cdecl * p_AMDObserveNotification_t)(SOCKET handle, CFStringRef name);
typedef void(*ListenForNotificationsCallback_t)(CFStringRef, unsigned int user_data);
typedef mach_error_t (__cdecl * p_AMDListenForNotifications_t)(SOCKET handle, ListenForNotificationsCallback_t func, unsigned int user_data);
typedef mach_error_t (__cdecl * p_AMDSecurePostNotification_t)(SOCKET handle, CFStringRef name, CFStringRef userinfo);
typedef mach_error_t (__cdecl * p_AMDSecureShutdownNotificationProxy_t)(SOCKET handle);
typedef mach_error_t (__cdecl * p_AMDSecureListenForNotifications_t)(SOCKET handle, ListenForNotificationsCallback_t func, unsigned int user_data);
typedef mach_error_t (__cdecl * p_AMDSecureObserveNotification_t)(SOCKET handle, CFStringRef name);
typedef void (__cdecl * p_AMDSetLogLevel_t)(int level);


struct UCollator;
struct UTransliterator;
typedef int UErrorCode, UColAttribute, UColAttributeValue, UColBoundMode, UTransDirection;
typedef wchar_t UChar;


enum { U_PARSE_CONTEXT_LEN = 16 };

struct UParseError 
{
	int32_t        line;
	int32_t        offset;
	UChar          preContext[U_PARSE_CONTEXT_LEN];
	UChar          postContext[U_PARSE_CONTEXT_LEN];
};

enum 
{
	U_ZERO_ERROR =  0,    
};

#define U_FAILURE(x) ((x)>U_ZERO_ERROR)
#define U_SUCCESS(x) ((x)<=U_ZERO_ERROR)

typedef UCollator* (__cdecl * p_ucol_open_4_0_t)(const char * loc, UErrorCode * status);
typedef void (__cdecl * p_ucol_close_4_0_t)(UCollator * coll);
typedef int32_t (__cdecl * p_ucol_getSortKey_4_0_t)(const UCollator *  coll, const UChar * source, int32_t sourceLength, uint8_t * result, int32_t resultLength); 
typedef void (__cdecl * p_ucol_setAttribute_4_0_t)(UCollator * coll, UColAttribute attr, UColAttributeValue value, UErrorCode * status) 			;
typedef int32_t (__cdecl * p_ucol_getBound_4_0_t)(const uint8_t * source, int32_t  sourceLength, UColBoundMode boundType, uint32_t noOfLevels, uint8_t * result, int32_t resultLength, UErrorCode * status);
typedef UTransliterator* (__cdecl * p_utrans_openU_4_0_t)(const UChar * id, int32_t idLength, UTransDirection dir, const UChar * rules, int32_t rulesLength, UParseError * parseError, UErrorCode * pErrorCode);
typedef void (__cdecl * p_utrans_close_4_0_t)(UTransliterator * trans );
typedef void (__cdecl * p_utrans_transUChars_4_0_t)(const UTransliterator * trans,	UChar * text, int32_t * textLength, int32_t textCapacity, int32_t start, int32_t * 	limit, UErrorCode * status);