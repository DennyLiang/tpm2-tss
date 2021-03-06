//**********************************************************************;
// Copyright (c) 2015, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//**********************************************************************;

#include "tss2_tpm2_types.h"

#include "sample.h"
#include "sysapi_util.h"

//
// This function does a hash on a string of data.
//
UINT32 TpmHash( TPMI_ALG_HASH hashAlg, UINT16 size, BYTE *data, TPM2B_DIGEST *result )
{
    TSS2_RC rval;
    TPM2B_MAX_BUFFER dataSizedBuffer;
    UINT16 i;
    TSS2_SYS_CONTEXT *sysContext;

    dataSizedBuffer.size = size;
    for( i = 0; i < size; i++ )
        dataSizedBuffer.buffer[i] = data[i];

    sysContext = InitSysContext( 3000, resMgrTctiContext, &abiVersion );
    if( sysContext == 0 )
        return TSS2_APP_RC_INIT_SYS_CONTEXT_FAILED;

    INIT_SIMPLE_TPM2B_SIZE( *result );
    rval = Tss2_Sys_Hash ( sysContext, 0, &dataSizedBuffer, hashAlg, TPM2_RH_NULL, result, 0, 0);

    TeardownSysContext( &sysContext );

    return rval;
}


//
// This function does a hash on an array of data strings.
//
UINT32 TpmHashSequence( TPMI_ALG_HASH hashAlg, UINT8 numBuffers, TPM2B_DIGEST *bufferList, TPM2B_DIGEST *result )
{
    UINT32 rval;
    TSS2_SYS_CONTEXT *sysContext;
    TPM2B_AUTH nullAuth;
    TPMI_DH_OBJECT sequenceHandle;
    int i;
    TPM2B emptyBuffer;
    TPMT_TK_HASHCHECK validation;

    TSS2L_SYS_AUTH_COMMAND cmdAuthArray = { .count = 1, .auths= {{
        .sessionHandle = TPM2_RS_PW,
        .sessionAttributes = 0,
        .nonce={.size=0},
        .hmac={.size=0}}}};

    nullAuth.size = 0;
    emptyBuffer.size = 0;

    // Set result size to 0, in case any errors occur
    result->size = 0;

    sysContext = InitSysContext( 3000, resMgrTctiContext, &abiVersion );
    if( sysContext == 0 )
        return TSS2_APP_RC_INIT_SYS_CONTEXT_FAILED;

    rval = Tss2_Sys_HashSequenceStart( sysContext, 0, &nullAuth, hashAlg, &sequenceHandle, 0 );

    if( rval != TPM2_RC_SUCCESS )
        goto teardown;

    for( i = 0; i < numBuffers; i++ )
    {
        rval = Tss2_Sys_SequenceUpdate ( sysContext, sequenceHandle, &cmdAuthArray, (TPM2B_MAX_BUFFER *)&bufferList[i], 0 );

        if( rval != TPM2_RC_SUCCESS )
            goto teardown;
    }

    INIT_SIMPLE_TPM2B_SIZE( *result );
    rval = Tss2_Sys_SequenceComplete ( sysContext, sequenceHandle, &cmdAuthArray, ( TPM2B_MAX_BUFFER *)&emptyBuffer,
            TPM2_RH_PLATFORM, result, &validation, 0 );

teardown:
    TeardownSysContext( &sysContext );
    return rval;
}

