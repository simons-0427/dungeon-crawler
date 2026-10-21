#ifndef WEB_PERSIST_H
#define WEB_PERSIST_H

// Android's raylib fopen wrapper redirects writable relative paths to the app's
// private internal data directory. No browser filesystem synchronization is
// required for the native APK.
static inline void WebPersistLoad(const char *path) { (void)path; }
static inline void WebPersistSave(const char *path) { (void)path; }

#endif // WEB_PERSIST_H
