#include "gzip.h"
#include "zlib.h"

std::string gzip::compress(const std::string& data)
{
	if (data.empty()) {
		return "";
	}

	z_stream stream;
	memset(&stream, 0, sizeof(z_stream));

	stream.next_in = (Bytef *)data.c_str();
	stream.avail_in = (uInt)data.size();
	stream.total_out = 0;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;

	//只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
	if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16,
		8, Z_DEFAULT_STRATEGY) != Z_OK) {
		return "";
	}

	const int KBufLen = 1024;
	Byte buf[KBufLen];

	std::string result(data);
	int res = 0;

	bool isCompressOK = true;
	while (isCompressOK && stream.avail_out == 0) {
		memset(buf, 0, KBufLen * sizeof(Byte));
		stream.avail_out = KBufLen;
		stream.next_out = buf;

		res = deflate(&stream, Z_FINISH);
		switch (res) {
		case Z_NEED_DICT:
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
		case Z_STREAM_ERROR:
		case Z_BUF_ERROR:
			isCompressOK = false;
			break;
		default:
			if (res == Z_OK || res == Z_STREAM_END) {
				const int dataLen = KBufLen - stream.avail_out;
				if (dataLen > 0) {
					result.append((char*)buf, dataLen);
				}
			}
			break;
		}
	}

	res = deflateEnd(&stream);
	if (res != Z_OK || !isCompressOK) {
		return "";
	}

	return result;
}

std::string gzip::decompress(const std::string& data)
{
	if (data.empty()) {
		return "";
	}

	z_stream stream;
	memset(&stream, 0, sizeof(z_stream));

	stream.next_in = (Bytef *)data.c_str();
	stream.avail_in = (uInt)data.size();
	stream.total_out = 0;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;

	int res = inflateInit2(&stream, MAX_WBITS + 16);
	// inflateInit2(&strm, (15+32))
	//只有设置为MAX_WBITS + 16才能在解压带header和trailer的文本

	if (res != Z_OK) {
		return "";
	}

	const int KBufLen = 1024;
	Byte buf[KBufLen];
	memset(buf, 0, KBufLen * sizeof(Byte));

	std::string result;

	bool isUncompressOK = true;
	while (isUncompressOK && stream.avail_out == 0) {
		memset(buf, 0, KBufLen * sizeof(Byte));
		stream.avail_out = KBufLen;
		stream.next_out = buf;

		res = inflate(&stream, Z_SYNC_FLUSH);
		switch (res) {
		case Z_NEED_DICT:
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
		case Z_STREAM_ERROR:
		case Z_BUF_ERROR:
			isUncompressOK = false;
			break;
		default:
			if (res == Z_OK || res == Z_STREAM_END) {
				const int dataLen = KBufLen - stream.avail_out;
				if (dataLen > 0) {
					result.append((char*)buf, dataLen);
				}
			}
			break;
		}
	}

	res = inflateEnd(&stream);
	if (res != Z_OK || !isUncompressOK) {
		return "";
	}

	return result;
}
