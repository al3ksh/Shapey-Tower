// macos_icon.mm - ustawienie ikony Dock (macOS-only)
#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
extern "C" void SetDockIcon(const char* path){
    if(!path) return;
    @autoreleasepool {
        NSImage *img = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:path]];
        if(img){
            if([NSApplication sharedApplication]){
                [NSApp setApplicationIconImage:img];
            }
        }
    }
}
#endif
