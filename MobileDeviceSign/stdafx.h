#pragma once

#define MDS_API __stdcall

#ifndef MDS_EXPORTS
#define MDS_EXPORT __declspec(dllimport) MDS_API 
#else
#define MDS_EXPORT __declspec(dllexport) MDS_API 
#endif

extern "C" {
	namespace itunescrypt {
		void MDS_EXPORT cbk_generate_for_sha1(unsigned char * pSHA1, const unsigned char * pDeviceUniqueID, unsigned char * pSignature);
		void MDS_EXPORT cbk_generate_for_data(unsigned char * pItunesDB, size_t pItunesDBLen, const unsigned char * pDeviceUniqueID, unsigned char * pSignature);
		void MDS_EXPORT hash58_generate_key(const unsigned char *pFWID, unsigned char *pKey);
	};

}