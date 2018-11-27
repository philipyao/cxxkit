#ifndef __CXXKIT_ERRCODE_H__
#define __CXXKIT_ERRCODE_H__

namespace kit {

enum ErrcodeBaseDef {
    kErrcodeBase                = 0,
    kErrcodeBaseCommon          = kErrcodeBase - 1000,
    kErrcodeBaseOther           = kErrcodeBase - 2000,
    kErrcodeBaseFsm             = kErrcodeBase - 3000,
};

enum {
    kErrcodeOK                  = kErrcodeBase,
    kErrcodeInvalidArgs         = kErrcodeBase - 1,
    kErrcodeNullPtr             = kErrcodeBase - 2,
    kErrcodeNotFound            = kErrcodeBase - 3,
    kErrcodeOutOfRange          = kErrcodeBase - 4,
    kErrcodeMeetUplimit         = kErrcodeBase - 5,
};

enum ErrcodeCommon {
    kErrcodeCommonTest1         = kErrcodeBaseCommon - 1,
};

enum ErrcodeOther {
    kErrcodeOtherTest1          = kErrcodeBaseOther - 1,
};

enum ErrcodeFsm {
    kErrcodeFsmInvalidState     = kErrcodeBaseFsm - 1,
    kErrcodeFsmDuplicatedKind   = kErrcodeBaseFsm - 2,
};

} // namespace kit

#endif //__CXXKIT_ERRCODE_H__
