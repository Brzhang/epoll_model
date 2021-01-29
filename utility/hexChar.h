#ifndef HEXCHAE_H_
#define HEXCHAE_H_

#ifdef __cplusplus
extern "C" {
#endif

    void CharToHex(unsigned char *pchOut, const unsigned char *pchIn, int nLen);
    void HexToChar(unsigned char *pchOut, const unsigned char *pchIn, int nInLen);

#ifdef __cplusplus
}
#endif

#endif
