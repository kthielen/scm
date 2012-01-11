#include <scm/eval/Data.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/CTypeUtil.hpp>

#include <scm/parse/StreamFSM.hpp>
#include <scm/stream/embedded_script_stream.hpp>
#include <scm/str/Util.hpp>

#include <vector>
#include <fstream>
#include <sys/stat.h>

using namespace str;

namespace scm {

/*
	Resizable byte arrays
*/
struct VarByteArray : Value { typedef std::vector<unsigned char> VecRep; VecRep data; };
Value* MakeByteArray(Allocator* a) { return a->allocate<VarByteArray>(); }
unsigned int ByteArraySize(Value* v, EnvironmentFrame* env) { return assert_type<VarByteArray>(Eval(v, env))->data.size(); }

void ByteArrayWrite(Value* v, unsigned int i, unsigned int b, EnvironmentFrame* env)
{
	VarByteArray* vb = assert_type<VarByteArray>(Eval(v, env));
	if (vb->data.size() <= i) { vb->data.resize(i + 1); }
	vb->data[i] = static_cast<unsigned char>(b % 256);
}

unsigned int ByteArrayRead(Value* v, unsigned int i, EnvironmentFrame* env)
{
	VarByteArray* vb = assert_type<VarByteArray>(Eval(v, env));
	
	if (vb->data.size() <= i)
		throw std::runtime_error("Byte array read index out of bounds.");
	else
		return static_cast<unsigned int>(vb->data[i]);
}

Value* ByteArrayChop(Value* v, unsigned int sidx, unsigned int eidx, EnvironmentFrame* env)
{
	VarByteArray* vb = assert_type<VarByteArray>(Eval(v, env));
	if (sidx >= eidx || sidx >= vb->data.size()) return MakeByteArray(env->allocator());
	if (eidx >= vb->data.size()) eidx = vb->data.size() - 1;

	VarByteArray* ret = env->allocator()->allocate<VarByteArray>();
	ret->data.assign(vb->data.begin() + sidx, vb->data.begin() + eidx);
	return ret;
}

void ByteArrayClear(Value* v, EnvironmentFrame* env) { assert_type<VarByteArray>(Eval(v, env))->data.clear(); }

void ByteArrayDAppend(Value* v1, Value* v2, EnvironmentFrame* env)
{
	gcguard<VarByteArray> ev1 = assert_type<VarByteArray>(Eval(v1, env));
	VarByteArray*         ev2 = assert_type<VarByteArray>(Eval(v2, env));

	if (ev2->data.size() > 0)
		ev1->data.insert(ev1->data.end(), ev2->data.begin(), ev2->data.end());
}

void InitVarByteArrayEnv(EnvironmentFrame* env)
{
	env->Define("make-byte-array",    bindfn(env->allocator(), &MakeByteArray));
	env->Define("byte-array-size",    bindfn(env->allocator(), &ByteArraySize));
	env->Define("byte-array-write",   bindfn(env->allocator(), &ByteArrayWrite));
	env->Define("byte-array-read",    bindfn(env->allocator(), &ByteArrayRead));
	env->Define("byte-array-chop",    bindfn(env->allocator(), &ByteArrayChop));
	env->Define("byte-array-clear",   bindfn(env->allocator(), &ByteArrayClear));
	env->Define("byte-array-dappend", bindfn(env->allocator(), &ByteArrayDAppend));
}

/*
	Random access binary file I/O
*/
struct RandomAccessFile : Value { std::fstream file_data; };

Value* OpenRAFile(const std::string& fname, Allocator* alloc)
{
	// ensure the file exists
	{
		std::ofstream f(fname.c_str(), std::ios_base::out | std::ios_base::app);
		f.close();
	}

	// really open the file ...
	RandomAccessFile* ret = alloc->allocate<RandomAccessFile>();
	ret->file_data.open(fname.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);

	if (ret->file_data.is_open())
		return ret;
	else
		throw std::runtime_error("Couldn't open '" + fname + "' for random access binary I/O.");
}
void CloseRAFile(Value* v, EnvironmentFrame* env) { assert_type<RandomAccessFile>(Eval(v, env))->file_data.close(); }
void SeekRAFile(Value* v, unsigned int pos, EnvironmentFrame* env)
{
	RandomAccessFile* f = assert_type<RandomAccessFile>(Eval(v, env));
	f->file_data.seekg(pos);
	f->file_data.seekp(pos);
}
unsigned int TellRAFile(Value* v, EnvironmentFrame* env) { return assert_type<RandomAccessFile>(Eval(v, env))->file_data.tellg(); }
unsigned int RAFileSize(Value* v, EnvironmentFrame* env)
{
	RandomAccessFile* f = assert_type<RandomAccessFile>(Eval(v, env));
	unsigned int pos = f->file_data.tellg();
	f->file_data.seekg(0, std::ios_base::end);
	unsigned int len = f->file_data.tellg();
	f->file_data.seekg(pos);
	return len;
}

unsigned int IReadRAFile(Value* v, EnvironmentFrame* env)
{
	RandomAccessFile* f = assert_type<RandomAccessFile>(Eval(v, env));
	unsigned char     c = 0;
	f->file_data.read(reinterpret_cast<char*>(&c), 1);
	if (f->file_data.gcount() == 0)
	{
		f->file_data.clear();
		throw std::runtime_error("Can't read at EOF.");
	}

	return static_cast<unsigned int>(c);
}

Value* IReadRAFileArray(Value* v, unsigned int vsize, EnvironmentFrame* env)
{
	RandomAccessFile* f   = assert_type<RandomAccessFile>(Eval(v, env));
	VarByteArray*     ret = env->allocator()->allocate<VarByteArray>();
	ret->data.resize(vsize);

	if (vsize > 0)
		f->file_data.read(reinterpret_cast<char*>(&(*(ret->data.begin()))), vsize);

	return ret;
}

void OWriteRAFile(Value* v, unsigned int c, EnvironmentFrame* env)
{
	unsigned char b = static_cast<unsigned char>(c % 256);
	assert_type<RandomAccessFile>(Eval(v, env))->file_data.write(reinterpret_cast<char*>(&b), 1);
}

void OWriteRAFileArray(Value* v, Value* a, EnvironmentFrame* env)
{
	gcguard<RandomAccessFile> f  = assert_type<RandomAccessFile>(Eval(v, env));
	VarByteArray*             ar = assert_type<VarByteArray>(Eval(a, env));

	if (ar->data.size() > 0)
		f->file_data.write(reinterpret_cast<char*>(&(*(ar->data.begin()))), ar->data.size());
}

void InitRAFileIO(EnvironmentFrame* env)
{
	InitVarByteArrayEnv(env);

	env->Define("open-binary-file",  bindfn(env->allocator(), &OpenRAFile));
	env->Define("close-binary-file", bindfn(env->allocator(), &CloseRAFile));

	env->Define("seek", bindfn(env->allocator(), &SeekRAFile));
	env->Define("tell", bindfn(env->allocator(), &TellRAFile));
	env->Define("size", bindfn(env->allocator(), &RAFileSize));

	env->Define("read-byte",   bindfn(env->allocator(), &IReadRAFile));
	env->Define("read-array",  bindfn(env->allocator(), &IReadRAFileArray));
	env->Define("write-byte",  bindfn(env->allocator(), &OWriteRAFile));
	env->Define("write-array", bindfn(env->allocator(), &OWriteRAFileArray));
}

/*
	Standard file I/O
*/
#ifdef WIN32
	std::list<std::string> Ls(const std::string& dirname)
	{
		std::string dst = replace<char>(dirname, "/", "\\");

		if (dst.size() == 0)
			dst = "\\*";
		else if (dst[dst.size() - 1] != '\\')
			dst += "\\*";
		else
			dst += "*";

		typedef std::list<std::string> FSeq;
		FSeq contents;

		WIN32_FIND_DATA wfd; memset(&wfd, 0, sizeof(wfd));
		HANDLE hfs = FindFirstFile(dst.c_str(), &wfd);

		do
		{
            contents.push_back(std::string(wfd.cFileName));
		}
		while (FindNextFile(hfs, &wfd) != FALSE);
		FindClose(hfs);

		return contents;
	}

	void make_directory(const std::string& dirname)
    {
        std::string ndirname = str::replace<char>(dirname, "/", "\\");
        std::vector<std::string> dparts = str::csplit<char>(ndirname, "\\");
        std::string dbuf;

        for (std::vector<std::string>::const_iterator d = dparts.begin(); d != dparts.end(); ++d)
        {
            dbuf += *d + "\\";
            CreateDirectory(dbuf.c_str(), 0);
        }
    }

    bool IsDirectoryP(const std::string& dirname)
    {
        DWORD dwa = GetFileAttributes(dirname.c_str());

        if (dwa == 0xFFFFFFFF)
            return false;
        else
            return (dwa & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
    }
#else
    std::list<std::string> Ls(const std::string& dirname)
    {
		return std::list<std::string>();
/*
        using namespace boost::filesystem;

        path fspath(dirname);
        if (!exists(fspath)) return std::list<std::string>();

        typedef std::list<std::string> FSeq;
        FSeq contents;

        directory_iterator iter(fspath);
        directory_iterator end;

        while (iter != end)
        {
            contents.push_back(iter->leaf());
            ++iter;
        }

        return contents;
*/
    }

    void make_directory(const std::string& dirname) { system(std::string("mkdir -p \"" + dirname + "\"").c_str()); }

    bool IsDirectoryP(const std::string& dirname)
    {
		return false;
/*
        using namespace boost::filesystem;

        try
        {
            path fspath(dirname);
            if (!exists(fspath)) return false;

            directory_iterator iter(fspath);
            return true;
        }
        catch (...)
        {
            return false;
        }
*/
    }
#endif

void Import(std::istream* input, EnvironmentFrame* env) { while (*input) Eval(ReadSExpression(*input, *(env->allocator())), env); }

void Include(const std::string& fname, EnvironmentFrame* env)
{
    const Symbol* mlvs = alloc_symbol(env->allocator(), "#included-files");
    
    ConsPair* mods  = env->HasImmediateValue(mlvs) ? expr_cast<ConsPair>(env->Lookup(mlvs)) : 0;
    ConsPair* omods = mods;

    while (mods != 0)
    {
        if (String* s = expr_cast<String>(mods->head()))
        {
            if (s->value() == fname)
                return;
        }

        mods = expr_cast<ConsPair>(mods->tail());
    }

    // let the script know its path
    std::string     sp    = str::rsplit<char>(fname, "/").first;
    if (sp.empty()) sp = ".";
    sp += "/";

    Value*        spath = env->allocator()->allocate<String>(sp);
    const Symbol* spsym = alloc_symbol(env->allocator(), "script-path");

    if (env->HasImmediateValue(spsym))
        env->Set(spsym, spath);
    else
        env->Define(spsym, spath);

    // if we get here, it's OK to include the file
    // if the file doesn't exist on the filesystem, but there's an active package, try to extract it from the package
    ConsPair* new_modlist = env->allocator()->allocate<ConsPair>(env->allocator()->allocate<String>(fname), omods);
    if (!env->HasImmediateValue(mlvs)) env->Define(mlvs, new_modlist); else env->Set(mlvs, new_modlist);

    std::ifstream fs(fname.c_str());
    if (!fs.is_open()) {
        throw std::runtime_error("Unable to open the file '" + fname + "' for reading.");
    }

    string_pair p = rsplit<char>(fname, ".");

    if (p.second == "ssp") {
		stream::script_substitutions<char> subs;
		subs.pass_through_prefix = "(sprint \"";
		subs.pass_through_suffix = "\")";
		subs.inline_code_prefix  = "(sprint ";
		subs.inline_code_suffix  = ")";

		stream::embedded_script_stream<char> ess(fs, subs);
		Import(&ess, env);
//    } else if (p.second == "spkg") {
//        EvalPackage(fname, env);
    } else {
		Import(&fs, env);
    }
}

std::istream* LOpenFile(std::string file_name)
{
    std::ifstream* ret = new std::ifstream(file_name.c_str());
    if (!ret->is_open()) { delete ret; throw std::runtime_error("Unable to open the file '" + file_name + "'."); }
    return ret;
}

std::ostream* LWriteFile(std::string file_name)
{
    return new std::ofstream(file_name.c_str());
}

void LCloseFile(Value* v, EnvironmentFrame* env)
{
	// this won't be pretty ...
	Value* ev = Eval(v, env);

	if (IStream* is = expr_cast<IStream>(ev))
	{
		std::ifstream* ifo = dynamic_cast<std::ifstream*>(is->value());
		if (ifo == 0) throw std::runtime_error("Expected an input file stream to close.");
		ifo->close();
	}
	else if (OStream* os = expr_cast<OStream>(ev))
	{
		std::ofstream* ofo = dynamic_cast<std::ofstream*>(os->value());
		if (ofo == 0) throw std::runtime_error("Expected an output file stream to close.");
		ofo->close();
	}
	else
	{
		throw std::runtime_error("Expected a file to close.");
	}
}

int LFileModTime(const std::string& file_name)
{
    struct stat buf;
    
    if (stat(file_name.c_str(), &buf) == -1)
        return 0;
    else
        return buf.st_mtime;
}

void FDelFile(const std::string& file_name)
{
    unlink(file_name.c_str());
}

void FPrint(std::ostream* output, std::string data)
{
    if (output != 0) (*output) << data;
}

void FFlush(std::ostream* output)
{
    if (output != 0) (*output) << std::flush;
}

void InitFileIOStdEnv(EnvironmentFrame* env)
{
    Allocator* alloc = env->allocator();
    
    env->Define("ls",                     bindfn(alloc, &Ls));
    env->Define("make-directory",         bindfn(alloc, &make_directory));
    env->Define("is-directory?",          bindfn(alloc, &IsDirectoryP));
    env->Define("include",                bindfn(alloc, &Include));

    env->Define("open-file",              bindfn(alloc, &LOpenFile));
    env->Define("write-file",             bindfn(alloc, &LWriteFile));
    env->Define("close-file",             bindfn(alloc, &LCloseFile));
    env->Define("delete-file",            bindfn(alloc, &FDelFile));
    env->Define("file-modification-time", bindfn(alloc, &LFileModTime));
    env->Define("print",                  bindfn(alloc, &FPrint));
    env->Define("flush",                  bindfn(alloc, &FFlush));

    InitRAFileIO(env);
}

}
