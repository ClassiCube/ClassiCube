#ifndef CC_ERRORS_H
#define CC_ERRORS_H
/*
Provides a list list of internal ClassiCube errors
Copyright 2014-2022 ClassiCube | Licensed under BSD-3
*/

/* NOTE: When adding errors, remember to keep Logger.c up to date! */
enum CC_ERRORS {
	ERROR_BASE           = 0xCCDED000UL,
	ERR_END_OF_STREAM    = 0xCCDED001UL, /* Attempted to read more data than the stream provided */
	ERR_NOT_SUPPORTED    = 0xCCDED002UL, /* Operation is not supported in current state or at all */
	ERR_INVALID_ARGUMENT = 0xCCDED003UL, /* Invalid argument provided to a function */
	ERR_OUT_OF_MEMORY    = 0xCCDED004UL, /* Insufficient memory left to perform the requested allocation */

	OGG_ERR_INVALID_SIG  = 0xCCDED005UL, /* Bytes #1-#4 aren't "OggS" */
	OGG_ERR_VERSION      = 0xCCDED006UL, /* Byte #5 isn't 0 */
						 
	WAV_ERR_STREAM_HDR   = 0xCCDED007UL, /* Bytes #1-#4  aren't "RIFF" */
	WAV_ERR_STREAM_TYPE  = 0xCCDED008UL, /* Bytes #9-#12 aren't "WAV " */
	WAV_ERR_DATA_TYPE    = 0xCCDED009UL, /* Audio data type isn't 1 (PCM) */
	AUDIO_ERR_MP3_SIG    = 0xCCDED00AUL, /* Signature bytes are "ID3" */
	WAV_ERR_SAMPLE_BITS  = 0xCCDED00BUL, /* Bits per sample isn't 16 */

	SFD_ERR_NEED_DEFAULT_NAME = 0xCCDED00CUL,

	VORBIS_ERR_WRONG_HEADER     = 0xCCDED00DUL, /* Packet header doesn't match expected type */
	VORBIS_ERR_FRAMING          = 0xCCDED00EUL, /* Framing flag doesn't match expected value */
	VORBIS_ERR_VERSION          = 0xCCDED00FUL, /* Vorbis version isn't 0 */
	VORBIS_ERR_BLOCKSIZE        = 0xCCDED010UL, /* Invalid blocksize in identifier packet */
	VORBIS_ERR_CHANS            = 0xCCDED011UL, /* Either 0 or too many audio channels */
	VORBIS_ERR_TIME_TYPE        = 0xCCDED012UL, /* Time object has invalid type */
	VORBIS_ERR_FLOOR_TYPE       = 0xCCDED013UL, /* Floor object has invalid type */
	VORBIS_ERR_RESIDUE_TYPE     = 0xCCDED014UL, /* Residue object has invalid type */
	VORBIS_ERR_MAPPING_TYPE     = 0xCCDED015UL, /* Mapping object has invalid type */
	VORBIS_ERR_MODE_TYPE        = 0xCCDED016UL, /* Mode object has invalid type */
	VORBIS_ERR_CODEBOOK_SYNC    = 0xCCDED017UL, /* Codebook sync value doesn't match 0x564342 */
	VORBIS_ERR_CODEBOOK_ENTRY   = 0xCCDED018UL, /* Codebook specifies an entry outside its size */
	VORBIS_ERR_CODEBOOK_LOOKUP  = 0xCCDED019UL, /* Codebook has invalid entry lookup method */
	VORBIS_ERR_MODE_WINDOW      = 0xCCDED01AUL, /* Mode object has invalid windowing method */
	VORBIS_ERR_MODE_TRANSFORM   = 0xCCDED01BUL, /* Mode object has invalid transform method */
	VORBIS_ERR_MAPPING_CHANS    = 0xCCDED01CUL, /* Mapping object has invalid magnitude/angle combination */
	VORBIS_ERR_MAPPING_RESERVED = 0xCCDED01DUL, /* Mapping object has invalid reserved value */
	VORBIS_ERR_FRAME_TYPE       = 0xCCDED01EUL, /* Audio packet frametype isn't 0 */

	PNG_ERR_INVALID_SIG      = 0xCCDED01FUL, /* Stream doesn't start with PNG signature */
	PNG_ERR_INVALID_HDR_SIZE = 0xCCDED020UL, /* Header chunk has invalid size */
	PNG_ERR_TOO_WIDE         = 0xCCDED021UL, /* Image is over 32,768 pixels wide */
	PNG_ERR_TOO_TALL         = 0xCCDED022UL, /* Image is over 32,768 pixels tall */
	PNG_ERR_INVALID_COL_BPP  = 0xCCDED023UL, /* Invalid colorspace and bits per sample combination */
	PNG_ERR_COMP_METHOD      = 0xCCDED024UL, /* Image uses unsupported compression method */
	PNG_ERR_FILTER           = 0xCCDED025UL, /* Image uses unsupported filter method */
	PNG_ERR_INTERLACED       = 0xCCDED026UL, /* Image uses interlacing, which is unimplemented */
	PNG_ERR_PAL_SIZE         = 0xCCDED027UL, /* Palette chunk has invalid size */
	PNG_ERR_TRANS_COUNT      = 0xCCDED028UL, /* Translucent chunk has invalid size */
	PNG_ERR_TRANS_INVALID    = 0xCCDED029UL, /* Colorspace doesn't support translucent chunk */
	PNG_ERR_REACHED_IEND     = 0xCCDED02AUL, /* Image only has partial data */
	PNG_ERR_NO_DATA          = 0xCCDED02BUL, /* Image is missing all data */
	PNG_ERR_INVALID_SCANLINE = 0xCCDED02CUL, /* Image row has invalid type */

	ZIP_ERR_TOO_MANY_ENTRIES        = 0xCCDED02DUL, /* ZIP archive has too many entries */
	ZIP_ERR_SEEK_END_OF_CENTRAL_DIR = 0xCCDED02EUL, /* Failed to seek to end of central directory record */
	ZIP_ERR_NO_END_OF_CENTRAL_DIR   = 0xCCDED02FUL, /* Failed to find end of central directory record */
	ZIP_ERR_SEEK_CENTRAL_DIR        = 0xCCDED030UL, /* Failed to seek to central directory records */
	ZIP_ERR_INVALID_CENTRAL_DIR     = 0xCCDED031UL, /* Central directory record has invalid signature */
	ZIP_ERR_SEEK_LOCAL_DIR          = 0xCCDED032UL, /* Failed to seek to a local directory record */
	ZIP_ERR_INVALID_LOCAL_DIR       = 0xCCDED033UL, /* Local directory record has invalid signature */
	ZIP_ERR_FILENAME_LEN            = 0xCCDED034UL, /* ZIP entry filename is too long */

	GZIP_ERR_HEADER1   = 0xCCDED035UL, /* GZIP stream byte #1 isn't 0x1F */
	GZIP_ERR_HEADER2   = 0xCCDED036UL, /* GZIP stream byte #2 isn't 0x8B */
	GZIP_ERR_METHOD    = 0xCCDED037UL, /* GZIP stream uses unsupported compression method */
	GZIP_ERR_FLAGS     = 0xCCDED038UL, /* GZIP stream uses unsupported flags */

	ZLIB_ERR_METHOD    = 0xCCDED039UL, /* ZLIB stream uses unsupported compression method */
	ZLIB_ERR_FLAGS     = 0xCCDED03AUL, /* ZLIB stream uses unsupported flags */

	FCM_ERR_IDENTIFIER = 0xCCDED03BUL, /* FCM stream bytes #1-#4 aren't 0x0FC2AF40 */
	FCM_ERR_REVISION   = 0xCCDED03CUL, /* FCM stream byte #5 isn't 13 */

	LVL_ERR_VERSION    = 0xCCDED03DUL, /* LVL stream byte #1-#2 aren't 1874 */

	DAT_ERR_IDENTIFIER  = 0xCCDED03EUL, /* DAT stream bytes #1-#4 aren't 0x271BB788 */
	DAT_ERR_VERSION     = 0xCCDED03FUL, /* DAT stream byte #5 isn't 2 */
	DAT_ERR_JIDENTIFIER = 0xCCDED040UL, /* DAT stream bytes #6-#7 aren't 0xACED */
	DAT_ERR_JVERSION    = 0xCCDED041UL, /* DAT stream bytes #8-#9 aren't 0x0005 */
	DAT_ERR_ROOT_OBJECT = 0xCCDED042UL, /* DAT version 2 root value isn't an object */

	JAVA_ERR_INVALID_TYPECODE  = 0xCCDED043UL, /* Typecode is invalid or incorrect */
	JAVA_ERR_JSTRING_LEN       = 0xCCDED044UL, /* String length is too long */
	JAVA_ERR_JFIELD_CLASS_NAME = 0xCCDED045UL, /* Field classname type is invalid */
	JAVA_ERR_JCLASS_TYPE       = 0xCCDED046UL, /* ClassDescriptor type is invalid */
	JAVA_ERR_JCLASS_FIELDS     = 0xCCDED047UL, /* ClassDescriptor has too many fields */
	JAVA_ERR_JCLASS_ANNOTATION = 0xCCDED048UL, /* ClassDescriptor uses unsupported annotations */
	JAVA_ERR_JCLASSES_COUNT    = 0xCCDED049UL, /* Too many ClassDescriptors in stream */
	JAVA_ERR_JCLASS_REFERENCE  = 0xCCDED04AUL, /* Reference refers to non-existent ClassDescriptor */
	JAVA_ERR_JOBJECT_FLAGS     = 0xCCDED04BUL, /* Object class isn't deserialisable */
	JAVA_ERR_JVALUE_TYPE       = 0xCCDED04CUL, /* Value data type is invalid */

	SOCK_ERR_UNKNOWN_HOST = 0xCCDED04FUL, /* Host (e.g. "example.com") was unknown to the DNS server(s) */

	NBT_ERR_UNKNOWN   = 0xCCDED050UL, /* NBT tag has an unknown type */
	CW_ERR_ROOT_TAG   = 0xCCDED051UL, /* NBT root tag isn't a Compound tag */
	CW_ERR_STRING_LEN = 0xCCDED052UL, /* NBT string is too long */
	CW_ERR_UUID_LEN   = 0xCCDED053UL, /* Map UUID byte array length is not 16 */

	AL_ERR_INIT_DEVICE   = 0xCCDED054UL, /* Unknown error occurred creating OpenAL device */
	AL_ERR_INIT_CONTEXT  = 0xCCDED055UL, /* Unknown error occurred creating OpenAL context */

	INF_ERR_BLOCKTYPE    = 0xCCDED056UL, /* Block has invalid block type */
	INF_ERR_LEN_VERIFY   = 0xCCDED057UL, /* Block length checksum failed */
	INF_ERR_REPEAT_BEG   = 0xCCDED058UL, /* Attempted to repeat codewords before first code */
	INF_ERR_REPEAT_END   = 0xCCDED059UL, /* Attempted to repeat codewords after last code */
	INF_ERR_INVALID_CODE = 0xCCDED05AUL, /* Attempted to decode unknown codeword */
	INF_ERR_NUM_CODES    = 0xCCDED05BUL, /* Too many codewords specified for bit length */

	ERR_DOWNLOAD_INVALID = 0xCCDED05CUL, /* Unspecified error occurred downloading data */
	ERR_NO_AUDIO_OUTPUT  = 0xCCDED05DUL, /* No audio output devices are connected */
	ERR_INVALID_DATA_URL = 0xCCDED05EUL, /* Invalid URL provided to download from */
	ERR_INVALID_OPEN_URL = 0xCCDED05FUL, /* Invalid URL provided to open in new tab */

	NBT_ERR_EXPECTED_I8  = 0xCCDED060UL, /* Expected I8 NBT tag  */
	NBT_ERR_EXPECTED_I16 = 0xCCDED061UL, /* Expected I16 NBT tag */
	NBT_ERR_EXPECTED_I32 = 0xCCDED062UL, /* Expected I32 NBT tag */
	NBT_ERR_EXPECTED_F32 = 0xCCDED063UL, /* Expected F32 NBT tag */
	NBT_ERR_EXPECTED_STR = 0xCCDED064UL, /* Expected String NBT tag */
	NBT_ERR_EXPECTED_ARR = 0xCCDED065UL, /* Expected Byte Array NBT tag */
	NBT_ERR_ARR_TOO_SMALL= 0xCCDED066UL, /* Byte Array NBT tag length is < expected length */

	HTTP_ERR_NO_SSL      = 0xCCDED067UL, /* HTTP backend doesn't support SSL */
	HTTP_ERR_REDIRECTS   = 0xCCDED068UL, /* Too many attempted HTTP redirects */
	HTTP_ERR_RELATIVE    = 0xCCDED069UL, /* Unsupported relative URL format */
	HTTP_ERR_INVALID_BODY= 0xCCDED06AUL, /* HTTP message doesn't have Content-Length or use Chunked transfer encoding */
	HTTP_ERR_CHUNK_SIZE  = 0xCCDED06BUL, /* HTTP message chunk has negative size/length */
};
#endif
