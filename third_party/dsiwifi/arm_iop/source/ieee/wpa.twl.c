/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#include "wpa.h"

#include "utils.h"

#include "crypto/sha1.h"
#include "crypto/md.h"
#include "crypto/pkcs5.h"
#include "crypto/nist_kw.h"

static int PBKDF2_HMAC_SHA_1(const char* pass, const char* salt, int32_t iterations, uint32_t outputBytes, uint8_t* out)
{
    mbedtls_md_context_t sha1_ctx;
    const mbedtls_md_info_t *info_sha1;
    int ret;

    info_sha1 = mbedtls_md_info_from_type( MBEDTLS_MD_SHA1 );
    if( info_sha1 == NULL )
        return( 104 );

    if( ( ret = mbedtls_md_setup( &sha1_ctx, info_sha1, 1 ) ) != 0 )
    {
        ret = 1;
        return ret;
    }

    ret = mbedtls_pkcs5_pbkdf2_hmac( &sha1_ctx, (u8*)pass, strlen(pass), (u8*)salt,
                              strlen(salt), iterations, outputBytes, out );
    if( ret != 0 )
    {
        return( 102 );
    }

    mbedtls_md_free( &sha1_ctx );

    return(0);
}

static void get_arr_min(const uint8_t* a, const uint8_t* b, size_t len, uint8_t* out)
{
    int a_is_max = 0;
    for (int i = 0; i < len; i++)
    {
        if (a[i] == b[i]) continue;
        
        if (a[i] > b[i])
        {
            a_is_max = 1;
            break;
        }
        
        break;
    }
    
    memcpy(out, a_is_max ? b : a, len);
}

static void get_arr_max(const uint8_t* a, const uint8_t* b, size_t len, uint8_t* out)
{
    int a_is_max = 0;
    for (int i = 0; i < len; i++)
    {
        if (a[i] == b[i]) continue;
        
        if (a[i] > b[i])
        {
            a_is_max = 1;
            break;
        }
        
        break;
    }
    
    memcpy(out, a_is_max ? a : b, len);
}

// !! This takes about 3 seconds on the 3DS ARM11 !!
// Use the cached version in NVRAM if you can.
void wpa_calc_pmk(const char* ssid, const char* pass, u8* pmk)
{
    // Generate PMK
    PBKDF2_HMAC_SHA_1(pass, ssid, 4096, 32, pmk);
}

void wpa_decrypt_gtk(const u8* kek, const u8* data, u32 data_len, gtk_keyinfo* out)
{
    mbedtls_nist_kw_context kw_ctx;
    uint8_t gtk_dec[0x200];
    size_t decrypted_length;
    

    mbedtls_nist_kw_init(&kw_ctx);

    // Set KEK
    mbedtls_nist_kw_setkey(&kw_ctx, MBEDTLS_CIPHER_ID_AES, kek, 128, 0);

    // Unwrap GTK
    mbedtls_nist_kw_unwrap(&kw_ctx, MBEDTLS_KW_MODE_KW, 
                           data, data_len,
                           gtk_dec, &decrypted_length,
                           0x200);

    mbedtls_nist_kw_free(&kw_ctx);
    
    //hexdump(gtk_dec, 0x40);
    
    memset(out, 0, sizeof(gtk_keyinfo));
    
    int i = 0;
    while (i < decrypted_length)
    {
        u8 ent_type = gtk_dec[i++];
        u8 ent_size = gtk_dec[i++];
        
        const u8 expected_keytype[4] = {0x00, 0x0f, 0xAC, 0x01};
        
        if (ent_type == 0xDD && !memcmp(&gtk_dec[i], expected_keytype, 4) && (ent_size == 0x16 || ent_size == 0x26))
        {
            memcpy(out, &gtk_dec[i], ent_size);
        }
        i += ent_size;
    }
}

void wpa_calc_mic(const u8* kck, const u8* pkt_data, u32 pkt_len, u8* mic_out)
{
    uint8_t out[0x20];
    sha1_hmac(kck, 0x10, pkt_data, pkt_len, out);
    
    //hexdump(out, 16);
    memcpy(mic_out, out, 16);
}

void wpa_calc_ptk(const u8* dev_mac, const u8* ap_mac, const u8* dev_nonce, const u8* ap_nonce, const u8* pmk, ptk_keyinfo* ptk)
{
    uint8_t ptk_out[0x60];
    uint8_t mac_min[6];
    uint8_t mac_max[6];
    uint8_t nonce_min[32];
    uint8_t nonce_max[32];
    
    get_arr_min(dev_mac, ap_mac, 6, mac_min);
    get_arr_max(dev_mac, ap_mac, 6, mac_max);
    
    //hexdump(mac_min, 6);
    //hexdump(mac_max, 6);
    
    get_arr_min(ap_nonce, dev_nonce, 32, nonce_min);
    get_arr_max(ap_nonce, dev_nonce, 32, nonce_max);
    
    
    // Generate PTK
    char ptk_data[512];
    memset(ptk_data, 0, sizeof(ptk_data));
    
    int ptk_data_pos = 0;
    strcpy(ptk_data, "Pairwise key expansion");
    ptk_data_pos += strlen("Pairwise key expansion") + 1;
    memcpy(ptk_data + ptk_data_pos, mac_min, 6); ptk_data_pos += 6;
    memcpy(ptk_data + ptk_data_pos, mac_max, 6); ptk_data_pos += 6;
    memcpy(ptk_data + ptk_data_pos, nonce_min, 32); ptk_data_pos += 32;
    memcpy(ptk_data + ptk_data_pos, nonce_max, 32); ptk_data_pos += 32;
    ptk_data[ptk_data_pos] = 0; ptk_data_pos += 1; // key iteration
    
    sha1_hmac(pmk, 0x20, (u8*)ptk_data, ptk_data_pos, ptk_out);
    
    ptk_data[ptk_data_pos-1] = 1;
    sha1_hmac(pmk, 0x20, (u8*)ptk_data, ptk_data_pos, ptk_out + 0x14);
    
    ptk_data[ptk_data_pos-1] = 2;
    sha1_hmac(pmk, 0x20, (u8*)ptk_data, ptk_data_pos, ptk_out + 0x14*2);
    
    ptk_data[ptk_data_pos-1] = 3;
    sha1_hmac(pmk, 0x20, (u8*)ptk_data, ptk_data_pos, ptk_out + 0x14*3);
    
    
    memcpy(ptk, ptk_out, sizeof(ptk_keyinfo));
    //hexdump(ptk_out, 16);
}
