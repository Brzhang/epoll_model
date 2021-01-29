#include "hexChar.h"

char SupperToLower(char in)
{
    if(in >= 65 && in <= 90)
        return in + 32;
    else
        return in;
}

char LowerToSupper(char in)
{
    if(in >= 97 && in <= 122)
        return in - 32;
    else
        return in;
}

static const char *tohex = "0123456789ABCDEF";

void CharToHex(unsigned char *pchOut, const unsigned char *pchIn, int nLen)
{
    int i = 0;
    while(i < nLen)
    {
        *pchOut++ = tohex[pchIn[i] >> 4];
        *pchOut++ = tohex[pchIn[i] & 0x0F];
        ++i;
    }
    
    return;
}

void HexToChar(unsigned char *pchOut, const unsigned char *pchIn, int nInLen)
{
    int an=1,num=0,st,q,tmp,kk = 0;
    for(q = nInLen - 1,st = nInLen / 2 - 1; q >= 0,st >= 0; q--)
    {
        if(kk==2)
        {
            pchOut[st] = (unsigned char)num;
            st --;
            num = 0;
            an = 1;
            kk = 0;
        }
        if(LowerToSupper(pchIn[q]) >= 'A' && LowerToSupper(pchIn[q]) <= 'F')
            tmp = pchIn[q] - 'A' + 10;
        else
            tmp = pchIn[q] - '0';
        num += an * tmp;
        an *= 16;
        kk ++;
    }
    
    return;
}
