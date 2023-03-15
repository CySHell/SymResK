/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_SymResK,
    0x044ebd2f,0xcf00,0x4db2,0x88,0x7c,0xbf,0x63,0xce,0x38,0x65,0x25);
// {044ebd2f-cf00-4db2-887c-bf63ce386525}
