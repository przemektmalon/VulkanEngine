#pragma once
#include "PCH.hpp"

class File
{
private:
	std::fstream file;
	
public:
	File() { meta.size = 0; meta.fileMode = Mode(binary | in | out); }
	~File() { close(); }

	enum Mode
	{
		in = std::ios_base::in,
		out = std::ios_base::out,
		ate = std::ios_base::ate,
		app = std::ios_base::app,
		trunc = std::ios_base::trunc,
		binary = std::ios_base::binary
	};

	struct FileMeta
	{
		std::string path;
		std::string status;
		u64 size;
		Mode fileMode;
	} meta;

	std::fstream& fstream() { return file; }

	bool create(std::string&& pPath, Mode pFileMode = (File::Mode)(File::binary | File::in | File::out));
	bool create(Mode pFileMode = (File::Mode)(File::binary | File::in | File::out));

	bool open(std::string&& pPath, Mode pFileMode = (File::Mode)(File::binary | File::in | File::out));
	bool open(std::string& pPath, Mode pFileMode = (File::Mode)(File::binary | File::in | File::out));
	bool open(Mode pFileMode = (File::Mode)(File::binary | File::in | File::out));

	bool atEOF() {
		return file.eof();
	}

	bool isOpen() {
		return file.is_open();
	}

	void close() {
		file.close();
	}

	bool checkWritable() {
		return (meta.fileMode & out) != 0;
	}

	bool checkReadable() {
		return (meta.fileMode & in) != 0;
	}

	s64 getSize() {
		return meta.size;
	}

	void setPath(std::string pPath) {
		meta.path = pPath;
	}

	// Writing functions

	void write(void* data, u32 size);

	template<class T>
	void write(T val);

	template<class T>
	void writeArray(T* val, u32 length = 1);

	void writeStr(std::string& string, bool includeTerminator = true, bool writeCapacity = true);

	void writeStr(std::string& string, bool includeTerminator = true);

	// Reading functions

	void read(void* data, u32 size);

	template<class T>
	void read(T& val);

	template<class T>
	void readArray(T* val, u32 length = 1);

	void read(std::string& string, u32 length);

	void readStr(std::string& string, char delim = '\n', int size = -1);

	void readFile(void* buffer);

	char peekChar();

	char pullChar();
};

template<class T>
void File::read(T & val)
{
	file.read((char*)&val, sizeof(T));
}

template<class T>
void File::readArray(T * val, u32 length)
{
	file.read((char*)val, sizeof(T)*length);
}

template<class T>
inline void File::write(T val)
{
	file.write((char*)&val, sizeof(T));
}

template<class T>
void File::writeArray(T * val, u32 length)
{
	file.write((char*)val, sizeof(T)*length);
}