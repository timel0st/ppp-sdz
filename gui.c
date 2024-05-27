#include "gui.h"

/* 
    Locates Graphical Output Protocol and writes its handle to pgop
    Returns TRUE on succss, FALSE on failure
*/
boolean_t locate_gop(efi_gop_t **pgop) {
    efi_guid_t gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_status_t status;
    status = BS->LocateProtocol(&gopGuid, NULL, (void**)pgop);
    if(!EFI_ERROR(status) && *pgop) {
        return TRUE;
    } else {
        fprintf(stderr, "unable to get graphics output protocol\n");
        return FALSE;
    }
}

int set_video_mode(efi_gop_t *gop, display_t *d) {
    efi_status_t status;
    if (locate_gop(&gop)) {
        status = gop->SetMode(gop, 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
        if(EFI_ERROR(status)) {
            fprintf(stderr, "unable to set video mode\n");
            return 0;
        }
        // set up destination buffer
        d->lfb = (uint32_t*)gop->Mode->FrameBufferBase;
        d->width = gop->Mode->Information->HorizontalResolution;
        d->height = gop->Mode->Information->VerticalResolution;
        d->pitch = sizeof(unsigned int) * gop->Mode->Information->PixelsPerScanLine;
        return 1;
    }
    return 0;
}