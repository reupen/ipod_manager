# iPod manager for foobar2000 changelog

## 0.7.0

* Added support for iPod nano 7G software versions later than 1.0.2
* Added identification of iPod nano 7G and later iPod shuffle 4G models
* Updated iTunesDB file format
* Fixed corruption of smart playlists with nested rules
* Fixed various clipped labels in preferences and other dialog boxes
* Reorganised conversion preferences and clarified some setting labels
* Disabled support for iOS device sync pause/cancel requests due to instability caused
* Compiled with Visual Studio 2017 15.3
* Compiled with latest foobar2000 SDK (foobar2000 1.3+ now required)
* Dropped official support for Windows XP and Vista

## 0.6.9.x

### 0.6.9.7
* Added support for iOS device sync pause/cancel requests
* Fixed: Files downloaded in-device should no longer generate errors during a sync
* Fixed: Compatibility with iTunes 10.5 beta 2

### 0.6.9.6
* Added option to apply ReplayGain gain during conversions

### 0.6.9.5
* Fixed regression in 0.6.9.2: podcasts on iOS devices may have not been grouped correctly

### 0.6.9.4
* Worked around a bug in Apple libraries used for mobile device support which sometimes caused a crash on immediate exit after starting foobar2000

### 0.6.9.3
* Fixed compatibility with devices with an anomalous USB serial number (only one seen)

### 0.6.9.2
* Database format updates
* Fixed: artwork was not displayed on some devices since 0.6.9.0
* Added "Verify settings" command in conversion options
* Added advanced preferences setting to change the folder used for temporary files during file conversions
* Misc changes/fixes

### 0.6.9.1
* Further increased JPEG compression quality parameter for JPEG artwork formats (so far these are only on iPads)
* Fixed: MP3 lyrics support was broken under foobar2000 1.1.6
* Format conversion improvements:
    * Improved settings page with encoder profiles (LAME MP3, Nero AAC and ffmpeg Apple Lossless out-of-the-box)
    * Added support for 32 bps encoder input
    * Added support for automatic 5.1 channel down-mixing to stereo

### 0.6.9.0
* Allowed connection to iOS devices when a respective USB device cannot be found (fixes compatibility with some wireless sync apps that patch Apple Mobile Device Support)
* Fixed artwork scaling for some iPad artwork formats
* Increased JPEG compression quality parameter for JPEG artwork formats (so far these are only on iPads)
* Added support for MP4 Apple podcast chapters
* Fixed issues with podcasts downloaded in-device on iOS devices
* Fixed audiobook/podcast playback positions were lost on iOS 4.3 devices
* Fixed: The date of podcasts transferred by the component were usually a day out
* Fixed: "Updating library..." on iOS 4.3 devices may have taken longer than normal
* Allowed use of external/hooked metadata fields in metadata remappings
* Miscellaneous fixes/changes

## 0.6.8.x

### 0.6.8.9
* Updated SQLite library
* Added recognition of new models

### 0.6.8.8
* Bug fixes

### 0.6.8.7
* Improvements to progress dialogs
* Database format updates
* Excluded books from sync operations
* Added discnumber and tracknumber to podcast sort criteria (only affects some devices)
* Misc changes

### 0.6.8.6
* Fixed a bug introduced in 0.6.8.5 which could result in a deadlock during write commands

### 0.6.8.5
* Shuffle 3G VoiceOver support should now respect the setting to enable it in the device settings in iTunes
* Support for iPod nano 6G (requires updated iPhoneCalc.dll)
* Fixed syncing with iOS 4 devices if attempted when the device is playing music
* Improved SQLite database writing speed
* Misc fixes / changes

### 0.6.8.4
* Support for iPhone 4 / iPod touch 4G / iPad (requires iPhoneCalc.dll)

### 0.6.8.3
* Improved artwork compatibility with recent/future devices

### 0.6.8.2
* Added option to forcibly sort playlists (enabled by default, only affects some models)
* Added support for setting date released field (uses RELEASE DATE or DATE as the source)
* Bug fixes

### 0.6.8.1
* Attempt to work around iOS 4 issue during artwork updates

### 0.6.8.0
* Compatibility with iOS 4 for iPod touch second and third generation, iPhone 3G and 3GS
* iOS device support requires Apple Mobile Device Support and Apple Application Support from iTunes 9.1 or newer.

## 0.6.7.x

### 0.6.7.9
* Now uses Microsoft Media Foundation for video thumbnails on Windows 7 (on Vista it will attempt to use it, falling back to DirectShow)
* Changes to sparse artwork implementation

### 0.6.7.8
* Added VoiceOver support for iPod shuffle 3G (currently forced on).
* Various bug fixes
* M4V files should no longer be removed during a sync
* Fixed an issue relating to the order of playlists when displayed in foobar2000

### 0.6.7.7
* Improved mobile device detection mechanism

### 0.6.7.6
* Fixed broken compatibility with older devices in 0.6.7.5

### 0.6.7.5
* Fixes for read-only iPad support

### 0.6.7.4
* Database format updates

### 0.6.7.3
* Further fix for iTunes 9.1 iPhone/iPod touch compatibility

### 0.6.7.2
* Restored compatibility with iTunes 9.0 for iPhone/iPod touch

### 0.6.7.1
* Fixed compatibility with iTunes 9.1 for iPhone/iPod touch

### 0.6.7.0
* Improved support for content ratings
* Added support for sending tracks as podcasts (set either ''MEDIA KIND'' or ''GENRE'' to *Podcast*)

## 0.6.6.x

### 0.6.6.9
* Fixed: Files with a leading space in their name could not be played on some devices
* Worked around other apparent malformed XML issues in property lists

### 0.6.6.8
* Rewrote code that interacts with Apple mobile device related libraries
* Fixed compatibility with older iPod models connected over firewire (broken in 0.6.6.4)
* Made use of sort order (*SORTORDER) metadata fields opt-in
* Misc changes

### 0.6.6.7
* Worked around a problem where some Nano 5G property lists had non-Base64 encoded data in a `<data>` tag, which prevented syncing such devices

### 0.6.6.6
* Reduced memory usage when writing the iPod database (better performance in low RAM, large database scenarios)

### 0.6.6.5
* Some podcast related bug fixes on iPhone/iPod touch devices
* Added file system explorer, allows you to browse the device file system and copy files from the device
* Added remappings for sort fields

### 0.6.6.4
* SoundCheck support for newer iPod shuffle models
* Some bug fixes for iPod shuffle 3G
* When the device properties cannot be retrieved (i.e. XP non-admin) write operations are now disabled
* Improved the properties displayed for iPod touch/iPhone
* Improved behaviour when playing tracks in foobar2000 on an iPod touch/iPhone shortly after foobar2000 startup (i.e. from 'Resume playback after restart')
* Added support for foobar2000 1.0 artwork sources
* Improvements to metadata caching mechanisms
* Bug fixes / misc changes

### 0.6.6.3
* Fixed crash using context menu sync command

### 0.6.6.2
* Removed dependencies on MobileDeviceSign/zlib libraries
* Added support for the playlist field in the smart playlist editor
* Fixed: Recover orphaned files causes issues where the case of file paths in the database do not match the actual case in the filesystem
* Moved 'iPod/Video properties' to 'Tagging/iPod tag editor'
* Added the ability to set the media type to "Audiobook" via the iPod tag editor
* Added support for the foobar2000 1.0 context menu
* When a remapping is used, the corresponding xxxSORTORDER field is no longer used
* Bug fixes

### 0.6.6.1
* Fixed: Rare crash on exit with iPod touch/iPhone
* Fixed: Problem writing SQLite databases under specific circumstances (iPod touch/iPod nano 5G/iPhone)
* Slightly faster SQLite database writing (iPod touch/iPod nano 5G/iPhone)
* Fixed an issue grouping tracks without ALBUM fields during conversion, added better handling for blank ALBUM fields
* Fixed some issues with relative artwork paths

### 0.6.6.0
* Added working support for ARTISTSORTORDER, COMPOSERSORTORDER, TITLESORTORDER, ALBUMARTISTSORTORDER, SHOWSORTORDER, ALBUMSORTORDER fields
* Added a remapping for COMMENT
* Added a workaround for lyrics field name issues for converted tracks
* No longer allows % characters in filenames (caused issues with iPhone/iPod touch 3.x firmware)
* Fixed: Empty smart playlists did not show up on an iPhone/iPod touch

## 0.6.5.x

### 0.6.5.9
* Fixed an issue reading play counts/ratings/etc. from an iPod touch/iPhone. As a side effect, the 'Manage contents' window will not reflect these on an iPhone/iPod touch until the database has been rewritten.
* Added a remapping for ALBUM ARTIST
* Fixed an issue syncing tracks which were converted from a file format storing track/disc numbers as text to a file format storing them as integers

### 0.6.5.8
* Fixed last modified time reporting on iPhone/iPod touch
* Fixed: race condition which prevented an iPhone/iPod touch from being detected sometimes at startup
* `<del>`Added support for ARTISTSORTORDER, COMPOSERSORTORDER, TITLESORTORDER, ALBUMARTISTSORTORDER, SHOWSORTORDER, ALBUMSORTORDER fields`</del>`
* Fixed: iPhone/iPod touch properties could not be displayed after some idle time
* Improvements to warnings/errors window displayed after running commands
* Improved speed of file reading from iPod touch/iPhone
* Bug fixes

### 0.6.5.7
* Database writer bug fixes
* Fixed: Errors writing a database to a blank Nano 5G.
* Database sort fields are now determined when adding files or when running the update metadata command (previously: when writing the database).

### 0.6.5.6
* Database writer bug fixes

### 0.6.5.5
* Improved support for iPhone/iPod touch OS 3.1
* Improved support for new iPod models (untested)

### 0.6.5.4
* Fixed: SoundCheck adjustment setting couldn't be changed in 0.6.5.3
* Fixed an issue with date fields on iPhone/iPod touch OS 3.0
* Fixed an issue related to Podcasts on iPhone/iPod touch

### 0.6.5.3
* Added support for converting files above a certain bitrate
* Added support for setting MEDIA KIND field to "audiobook" to mark any file type as an audiobook
* Fixed an issue importing data from iPhone/iPod touch play counts file
* Misc. bug fixes / changes

### 0.6.5.2
* Artwork for videos on iPod touch/iPhone is now correctly set
* Tracks with a sample rate above 48kHz are now correctly converted and/or resampled
* Some optimisations to SQLite database writer

### 0.6.5.1
* Fixed: XML PlayCounts.plist files were not read
* Various bug fixes to database writers

### 0.6.5.0
* Fixed a few bugs in bplist/PlayCounts.plist reader

## 0.6.4.x

### 0.6.4.9
* Added support for iPhone/iPod touch OS 3.0
* Fixed crash in iPod devices window/panel with corrupted database
* More database format updates

### 0.6.4.8
* zlib library now dynamically loaded
* iPhone 3GS now recognised (same 3.x software limitations apply)
* Fixed some issues with coverflow artwork on iPhone/iPod touch, especially with non-square artwork
* iTunes[C]DB format updates

### 0.6.4.7
* Fixed an issue updating an iPod touch/iPhone with iTunesCDB database

### 0.6.4.6
* Added support for iTunesCDB database files

### 0.6.4.5
* Initial attempt at Shuffle 3G support

### 0.6.4.4
* Bug fix

### 0.6.4.3
* Split "add missing artwork" functionality from 'Update metadata' command into a separate 'Update artwork' command
* Hashes are now used to track artwork changes, and 'Update artwork' now updates artwork if it has changed
* Shortcut menu iPod commands are now only shown when relevant
* Bug fixes

### 0.6.4.2
* Some changes to the order of events when syncing so that it catches items modified by playback data synchronisation.
* Leading A is ignored alongside The when respective option is enabled to avoid problems on iPod touch / iPhone
* Track number / Disc number also when checking for identical tracks already with same file size / modified date / artist / album / title to avoid certain fringe cases
* Option to Eject iPod when synchronisation has finished
* Should now regenerate metadata cache file if it becomes corrupt

### 0.6.4.1
* API updates
* Fixes for sync'ing playback statistics from foobar2000 to the iPod
* Fixed crash when attempting to open video properties on files that do not have any file info available (i.e. unsupported format)

### 0.6.4.0
* Synchronise iPod now leaves podcasts already on the device alone
* Fixed: iPod touch/iPhone devices were not listed under iPod devices
* Expanded properties displayed for iPhone
* Added "Send to playlist" shortcut menu command for items in iPod devices
* Added count of tracks to add and remove to sync preview
* Added option to select between track and album gain for SoundCheck value
* Misc changes/bug fixes

## 0.6.3.x

### 0.6.3.9
* Fixed an issue where iTunes would always recopy certain podcasts after using the component (some database format updates as a result)
* Added support for 'Properties' on iPhone/iPod touch
* Added drive space information to 'Properties'
* Can now use foobar2000 artwork reader without specifying a script
* Prefixed various console messages with "iPod manager: "

### 0.6.3.7
* Fixed: Podcast played state was reset after using the component (fix untested on mobile devices)
* Fixed: 'Bookmark' positions were not saved correctly
* Fixed: Subtitle field was not preserved by the component
* Fixed some inconsistencies with forward / backward slashes on mobile devices. May be some fallout as a result (bugs and possibly rereading of metadata from files on the device).
* Fixed path length limit not being applied correctly to files sent to mobile devices.

### 0.6.3.6
* Fixed an issue with not reading volume free space / capacity correctly on iPod touch / iPhone

### 0.6.3.5
* Fixed an issue with adding files existing on the iPod touch / iPhone drive not working (used mainly by 'recover orphaned files')

### 0.6.3.4
* Added support for iPod touch / iPhone with 2.x software.

### 0.6.3.0
* Fix for artwork support on nano 4G

## 0.6.2.x

### 0.6.2.9
* Compatibility fix for iPod nano 4G

### 0.6.2.8
* Recover orphaned files creates a foobar2000 playlist with the recovered files
* Send files will list skipped duplicates at the end of the process
* Updated recognition of iPod models in Properties, should now work for new shuffles/nano 4g/classic 120gb.

### 0.6.2.7
* Added debug commands to backup the iTunesDB and ArtworkDB database files from the iPod.
* Improved handling of low free space on iPod conditions.

### 0.6.2.6
* fixed an issue where 'ReplayGain scan converted files' did not work correctly
* 'Update metadata on iPod' checks the file on the iPod for embedded art if the source file no longer exists.

### 0.6.2.4
* 0.6.2.4: fixed an issue where on-the-fly conversion may fail in some circumstances.

### 0.6.2.3
* 0.6.2.3: hot-fix for 0.6.2.2

### 0.6.2.2
* 0.6.2.2: tweaked sync behaviour on iPod touch/iPhone
* 0.6.2.2: added charging status to "Properties" on iPod classic/nano 3G
* 0.6.2.2: added popup version of devices panel.

### 0.6.2.1
* 0.6.2.1: hot-fix for 0.6.2.0

### 0.6.2.0
* Reorganised preferences, and added help pages
* Added support for wildcards in artwork source script, and removed the requirement to specify the file extension
* Added a fix so that sync iPod works better on the iPod touch / iPhone for new files sent using the component
* Fixed an issue that may have caused the component to lose info in dopdb, affecting the sync of converted files
* Added DirectShow based video thumbnail creator
* Fixed issue where similar entries (e.g. The Simpsons and Simpsons) would cause many entries on the iPod
* Some clean-up of code and misc fixes

## 0.6.1.1
* Fixed a bug with artwork on iPod touch/iPhone introduced in version 0.6.1.2

## 0.6.1
* Fixed various bugs when using the "Stop" button during operations
* Attempt to fix transparent artwork on iPod touch / iPhone in cover flow view
* Rewrote some code

## 0.6.0
* Added preview for "Synchronise iPod" and quiet mode to disable it
* Ratings set with Playback Statistics component are now sent to the iPod
* Fixed behaviour of "Synchronise iPod" when remembering playlist selections for playlists where there are multiple playlists with the same name
* Added a fix for lyrics not displaying on the iPod after using iTunes
* Removed "Load iPod library and playlists" command
* Removed "Refresh iPod library metadata" command. Use the context menu command instead
* Added "Recover orphaned tracks" command
* Misc. fixes / changes

## 0.5.9.x TEST
* Added support for ReplayGain scanning converted files (requires ReplayGain Scanner 2.0.6+)
* Added support for creating playlist folders in 'Browse iPod'
* Some improvements to 'Browse iPod' and 'iPod devices panel'
* Added option to control date added source
* Removed 'Remove playlists from iPod' command; use 'Browse/manage iPod' instead
* The component now generates/updates smart playlist contents
* Corrected behaviour of "Stop" button; now leaves iPod in the current state and updates the database.
* Fixed 'sync iPod' on iPod touch/iPhone (untested)
* Fixed behaviour when you remove tracks directly on an iPod touch / iPhone (untested)
* Added option to set dummy gapless data for tracks that foo_dop doesn't find gapless data
* Corrected some bugs in smart playlist editor with date fields
* Misc changes / fixes

## 0.5.8.x TEST
* Added iPod Devices Panel. Currently lists connected iPods and allows you to browse through their contents and eject them.
* Some speed-ups to database reader.
* Added support for multiple concurrent encodings to the converter
* Improved converter speed when source file reading is slow (WLAN)
* Files with MP4 extension without video content are renamed to m4a (fixes some issues on the iPod)

## 0.5.7.0 TEST
* Added support for foobar2000 built-in artwork reader

## 0.5.6.x TEST
* Added iPod touch/iPhone support
* Fixed a bug which caused an error when adding certain artwork image formats to the iPod
* Transcoding is now done via a temporary file on the local computer
* Some changes to sorting on the iPod
* Implemented "numbers last" sorting to keep the Classic/Nano 3G/Touch/iPhone happy (can be disabled).
* Fixed: menu actions would not work from the command line if foobar2000 wasn't already running
* Updated iPod model detections in "View iPod Device Information"
* Fixed some bugs with non-square artwork in version 0.5.5
* Added video file tagger
* Changed default encoder to Nero AAC encoder.
* Some other improvements and bug fixes

## 0.5.5 TEST
* Artwork formats are now retrieved from the iPod's XML property list (instead of being hardcoded). This will result in cropping being used in the correct places (specifically on Classic/Nano 3G) and  better future compatibility. Drawback: artwork will require administrative privileges on Windows XP (Windows Vista is OK)

## 0.5.4 TEST
* fixed bug with mixed up imported OTG playlists on Classic / Nano 3G
* some fixes to sparse artwork handling on 6G/Nano 3G iPods
* added support for gapless data from Nero MP4 (AAC) files from recent Nero encoders :-)
* improved gapless data scanner handling of MP3 files with garbage data before first MPEG frame
* changed the order that files sent to the iPod are added to the iPod's database
* some improvements to progress reporting
* added option to control sorting of iPod library playlist when using "Load library"
* improved a specific artifact in artwork introduced by gdiplus when scaling
* fixed some potential issues relating to duplicate handling
* misc changes / fixes

## 0.5.3 TEST
* Some bug fixes to playback data API

## 0.5.2 TEST
* added compilation mapping option
* restored 'Omit leading 'The' when sorting' option
* added API to receive play count etc. information.
* fixed bug reading OTG playlists

## 0.5.1 TEST
* fixed non-lowercase file extension problem on iPod
* fixed mis-reporting of iPod models in 'View iPod Device Info'
* shortened filename length limit from 31 characters to 12 characters so Music Quiz 2 works on iPod Classics / Nano 3Gs. if you want longer filenames there is a setting on the advanced prefs page, a setting of 23 is equivalent to the limit in previous versions.

## 0.5.0 TEST
* fixed playlist mix-up problem on Classic/Nano 3G (fix applies to new playlists only)

## 0.4.9 TEST
* fixed crash bug on adding artwork on Nano 3G/Classic

## 0.4.8 TEST
* improved artwork handling for Nano 3G and Classic (inc. artwork previews in split menus)
* faster database writing for large databases
* fixed non-workingness / crashing of remove playlists command when removing multiple playlists
* better error report dialogs
* fixed nano 3g / classic support from blank iPod

## 0.4.7 TEST
* added support for iPod Classic and Nano 3G (with some known issues)
* some UI refresh
* misc changes / fixes
* discontinued Windows 2000 support

## 0.4.6 TEST
* added gapless support for MP4/M4A AAC files with iTunes gapless data
* uses process exit code to detect if a transcode was successful
* added smart playlist editor
* some database writer changes for iTunes 7.3.1
* removed "omit leading 'the' when sorting" option again because it is currently a big mess on (at least) iPod Videos. It may return when Apple release a new firmware that's fully supporting all of the sort fields in the database.
* misc changes/fixes

## 0.4.5 TEST
* fixed hang in 0.4.4 if no metadata cache present
* fixed some artwork bugs in 0.4.4
* some more changes to artwork handling

## 0.4.4 TEST
* Improved speed of checking iPod library metadata cache up-to-date (not actually updating cache) by ~50%; worked around problem of re-read of all metadata when clocks change due to DST/summer time.
* Removed I/O bottleneck in iTunesDB etc. reader/parser; iTunesDB reader/parser speed improved by ~50% to ~90% (latter achieved when Windows has cached iTunesDB file).
* Some changes in artwork handling
* Added options to disable adding artwork and converting files
* Added option to automatically set gapless data for files sent to iPod
* Added 'Update iPod library metadata' command to context menu
* Removed 'Browse photos' command
* Added detected of iPod model and colour to 'View iPod device information'

## 0.4.3 TEST
* Added (untested) album art support for iPod Nano (1G/2G) and iPod Color/Photo (USB only)

## 0.4.2 TEST
* Added album art support for iPod Videos (requires GDI+ on Windows 2000)
* Fixed sorting on Composer menu
* MP3 gapless scanner handles files with zero padding better
* Sync'ing iPod no longer removes smart playlists

## 0.4.1 TEST
* Fixed bug which caused foo_dop to lose entries in the dopdb database
* Added gapless info scanner for MP3s

## 0.4.0 TEST
* Fixed bug which may cause transcoded files not to play properly on the iPod
* Remember playback position defaults to true for Movies and TV Shows
* Added simple info editor to iPod Browser
* Added 'Eject iPod' to menu, functions same as if ejected through 'Safely Remove Hardware' (see main page notes)
* Some work on implementing iTunes 7.1 database format changes
* Added fields IPOD_REMEMBER_PLAYBACK_POSITION, IPOD_SKIP_WHEN_SHUFFLING
* Fixed bug which made foo_dop not read/write some of the newer track string fields
* Re-added 'Omit leading The...' option (see main page notes)
* Some other changes...

## 0.3.9 TEST
* Removed "Use iPod sorting" option
* Added MP4 video sending (see [usage notes](https://wiki.yuo.be/dop:notes) for more info)
* compiled with 2007.02.04 foobar2000 SDK (may improve database writer speed and speed in general)

## 0.3.8 TEST
* Fixed iPod detection on Windows Vista
* Added limited, untested iPod Shuffle support
* Added photo database reader, and browser for photos on your iPod
* Fixed crash bug in Browse iPod

## 0.3.7 TEST
* Detects iPods by their Device Instance ID now. iPod connections/disconnections monitored in background thread (rather than on execution of every command)
* 'Preparation' of iPods is automatic now (for commands that add things to the iPod)
* Refuses to acknowledge Shuffles as iPods now ;-) (since foo_dop does not work with them)
* Sync dialog remembers names of playlists last synced, and has select all/deselect all in contextmenu. Browse dialog resizable.
* Will now convert formats not supported by the iPod with chapters (e.g. FLAC with embedded cuesheet)

## 0.3.6 TEST
* Added simple transcoder to convert unsupported file formats
* Added 'Browse iPod' command to view iPod contents
* Now stores a database on iPod - file dopdb in iPod_Control folder

## 0.3.5 TEST
* Added lyrics support (see [usage notes](https://wiki.yuo.be/dop:notes) for more info)
* Send playlist command now supports sending multiple playlists in one pass
* Added remove playlists command
* Added main menu command to sync with media library / playlists
* Fixed sync crash bug

## 0.3.4 TEST
* Added ability to send a playlist to your iPod

## 0.3.3 TEST
* Added field remappings for GENRE, COMPOSER fields
* Added "SoundCheck" adjustment feaure
* Added simple sync feature

## 0.3.2 TEST
* Added field remappings for ARTIST, TITLE, and ALBUM fields

## 0.3.1
* Vista compatibility again

## 0.3.0 TEST
* Removed unmount/mount commands
* Added better support for iTunes 7.0 iTunesDB; preserves new database fields and writes Album Artist / Keywords
* Changed metadata loading mechanisms for "Load library" etc.; metadata now loaded directly from files and cached in a file in the root folder of your iPod. First load of library will be slow.
* Added "Prepare new iPod" command; generates empty iTunesDB for formatted iPod (untested)
* Compiled with latest foobar2000 SDK (Vista compatibility)
* Misc. fixes / foobar2000 0.9.4 compatibility fixes

## 0.2.9
* Fixed problem with drive scanner on Windows 2000
* Fixed problem with directory selector when sending files
* Some improvements/fixes to iTunesDB writer
* Other changes/fixes

## 0.2.8 TEST
* Fixed some problems with sorting
* Now "Remeber playback position" is disabled for new tracks sent using foo_dop
* Fixed problem with date added/date modified fields in database
* Renamed "Rewrite iPod database" to "Reload iPod library metadata from files"
* Added "Rewrite iPod database" - just reads database, and writes it back again
* iTunes columns data is preserved now
* Reload library metadata preserves metadata on files that have no metadata
* Added option to use more normal iPod style sorting
* New drive scanner - faster than old one

## 0.2.7
* Fixed bad sort order after send/remove file

## 0.2.6
* Fixed rewrite library

## 0.2.5
* Changes to writing mechanism: more robust; only keeps single backup of database now (iTunesDB.dop.backup)
* Now can not run multiple iPod operations simulataneously
* Play Counts file data is merged into iTunesDB on write operations and removed
* Added descriptions for iPodService errors
* Prevent metadata rereading from WAV files on rewrite database
* Compiled with foobar2000 2006-06-15 SDK
* Added Show, TV Network, Episode field reading for TV Shows (untested)
* Added filetype checks for sending files to the iPod (file extension only currently, doesn't consider codec)
* Other changes/fixes.

## 0.2.4 TEST
* Fixed some Podcast playlist writing bugs
* Added SUBTITLE field reading/writing (for podcasts)

## 0.2.3 TEST
* Fixed a bug in playlist reader (may have caused wrong sort order / podcast playlists showing up as normal playlists after write)
* Added reading / writing of kind/filetype

## 0.2.2 TEST
* iTunes 4.9 (+) database reading/writing
* Preserves more stuff (Chapter data, podcasts, some random fields...)
* On-The-Go playlists preserved as normal playlists
* Some bugs fixed

## 0.2.1
* Changed path limit to 55 characters

## 0.2.0
* Fixed couple bugs from 0.1.9

## 0.1.9
* Misc fixes / changes

## 0.1.8
* Fixed filename truncation
* Misc fix

## 0.1.7
* Added send files to iPod feature
* Added remove files from iPod
* Added Mount/Unmount iPod
* Misc changes / fixes

## 0.1.6
* Fixed writing RATING field

## 0.1.5
* Added iPod database rewrite feature

## 0.1.4
* Fixed GENRE field reading

## 0.1.3
* Bug fix for 0.1.2

## 0.1.2
* Added support for reading On The Go playlists

## 0.1.1
* Fixed a bug related to playlist loading
* Some other minor fixes

## 0.1
* Initial release

