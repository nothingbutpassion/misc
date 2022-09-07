#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <utility>
#include <string>
#include <vector>
#include "vpu_codec.h"


using namespace std;


#define MAX_BUFFER_SIZE 1024*1024*16


enum FileType {
    UNKNOW_FILE,
    YUV_FILE,
    JPEG_FILE,
    H264_FILE
};

enum WorkType {
    ENCODING,
    DECODING,
    TRANSCODING
};


const char* USAGE = "Usage:\n" 
                    "  vpu_codec_test -i <input-file> -o <output-file> [-w <picture-width> -h <picture-height> [-g <gop-size>] [-b <bitrate>] [-p <loop-count>] ]\n"
                    "Example:\n"
                    "  1) Encoding:    vpu_codec_test -i 539.yuv -o 539.jpg -w 1280 -h 800\n"
                    "  2) Decoding:    vpu_codec_test -i 539.jpg -o 539.yuv\n"
                    "  3) Transcoding: vpu_codec_test -i 539.jpg -o 539.264\n"
                    "  4) Transcoding profiling: vpu_codec_test -i 539.jpg -o 539.264 -w 1280 -h 800 -p 100\n"
                    "Notes:\n"
                    "  1) File with suffix '.yuv' (or '.YUV') is regarded as YUV file\n"
                    "  2) File with suffix '.jpg' (or '.JPG', or 'jpeg', or '.JPEG') is regarded as JPEG file\n" 
                    "  3) File with suffix '.264' (or '.h264', or 'H264') is regarded as H.264 file\n"
                    "  4) If <input-file> is a directory, it must contains the corresponding files (whose type is decided by directory name suffix)\n"
                    "  5) For encoding, options '-w' and '-h' must be specified\n"
                    "  6) For H.264 encoding, option '-g' specify the GOP size(0 = only first picture is I, 1 = all I frames, 2 = IPIP, 3 = IPPIPP, and so on)\n"
                    "  7) For H.264 encoding, option '-b' specify the bitrate(Unit: kbps, 0 = auto)\n"
                    "  8) If option '-p' is specified, will do transcoding profiling. In which case, options '-w' and '-h' must also be specified\n";


#define show_info(...)  fprintf(stdout, __VA_ARGS__)
#define show_error(...) fprintf(stderr, __VA_ARGS__)
#define show_error_exit(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); } while (0)
#define show_usage_exit() show_error_exit(USAGE)


struct TimeTrack { 
    uint64_t now() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec*1000 + tv.tv_usec/1000;
    }
    void mark() {
        timeval tv;
        gettimeofday(&tv, NULL);
        start =  tv.tv_sec*1000 + tv.tv_usec/1000;
    }
    uint64_t duration() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec*1000 + tv.tv_usec/1000 - start;
    }
    
private:
    uint64_t start;
};


FileType get_file_type(const string& file) {
    size_t pos = file.rfind(".");
    if (pos == string::npos) {
        return UNKNOW_FILE;
    }

    string suffix = file.substr(pos+1);
    if (suffix == "jpg" || suffix == "JPG" || suffix == "jpeg" || suffix == "JPEG") {
        return JPEG_FILE;
    }
    
    if (suffix == "yuv" || suffix == "YUV") {
        return YUV_FILE;
    }

    if (suffix == "264" || suffix == "h264" || suffix == "H264") {
        return H264_FILE;
    }

    return UNKNOW_FILE;
}


bool create_dir(const string& dir) {
	int err = mkdir(dir.c_str(), S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
	if (err && errno != EEXIST) {
	    show_error_exit("Can't create dir: %s\n", dir.c_str());	
	}
	return true;
}

bool is_dir(const string& path) {
	struct stat st;
	int err = stat(path.c_str(), &st);
	if (!err && S_ISDIR(st.st_mode)) {
		return true;
	}	
	return false;
}

bool is_file(const string& path) {
	struct stat st;
	int err = stat(path.c_str(), &st);
	if (!err && S_ISREG(st.st_mode)) {
		return true;
	}	
	return false;
}


string remove_suffix(const string& path) {
    string result;
	size_t pos = path.rfind(".");
	if (pos != string::npos) {
		result = path.substr(0, pos);
	}
	return result;
}

string base_name(const string& path) {
    string result = path;
	size_t pos = path.rfind("/");
	if (pos != string::npos) {
		result = path.substr(pos+1);
	}
	return result;
}



vector<string> dir_files(const string& dir) {
	vector<string> results;
    DIR* dp = opendir(dir.c_str());
	if (!dp) {
        show_error_exit("Can't open dir: %s\n", dir.c_str());
	}

	dirent* ent = NULL;
	while ((ent = readdir(dp)) != NULL) {
		string name(ent->d_name);
		if (name != "." && name != "..") {
            if (is_file(dir + "/" + name)) {
			    results.push_back(dir + "/" + name);
            }
		}
	}

    sort(results.begin(), results.end());
    closedir(dp);
	return results;
}


void read_file(const string& file, void* buf, size_t& len) {
    FILE* fp = fopen(file.c_str(), "rb");
    if (!fp) {
        show_error_exit("Can't open %s\n", file.c_str());
    }
    len = fread(buf, 1, MAX_BUFFER_SIZE, fp);
    if (len == 0) {
        show_error_exit("Read %s failed: error or end of file\n", file.c_str());  
    }
    fclose(fp);
}

void write_file(const string& file, void* buf, size_t len) {
    FILE* fp = fopen(file.c_str(), "wb");
    if (!fp) {
        show_error_exit("Can't open %s\n", file.c_str());
    }
    if (len != fwrite(buf, 1, len, fp)) {
        show_error_exit("Write %d bytes to %s failed\n", len, file.c_str());
    }
    fclose(fp);    
}


void alloc_buffer(void** in, void** out, void** other) {
    void** buffer[3] = {in, out, other};
    for (int i=0; i < 3; i++) {
        if (buffer[i]) {
            buffer[i][0] = calloc(1, MAX_BUFFER_SIZE);
            if (buffer[i][0] == NULL) {
                show_error_exit("calloc failed\n");
            }
        }
    }
}

void free_buffer(void* in, void* out, void* other) {
    void* buffer[3] = {in, out, other};
    for (int i=0; i < 3; i++) {
        if (buffer[i]) {
            free(buffer[i]);
        }
    }
}

void encode_one_frame(void* enc_handle, const string& infile, void* inbuf, size_t inlen, 
                            const string& outfile, void* outbuf, size_t outlen) {
    // Fill input buffer by reading input file
    read_file(infile, inbuf, inlen);
                       
    // Encode one frame
    vpu_enc_in enc_in;
    enc_in.buf = (char*)inbuf;
    enc_in.buflen = inlen;
    vpu_enc_out enc_out;
    enc_out.buf = (char*)outbuf;
    enc_out.buflen = outlen;
    if (vpu_encode(enc_handle, &enc_in, &enc_out) == -1) {
        show_error_exit("Encoding %s => %s failed: %s\n", infile.c_str(), outfile.c_str(), vpu_error());
        return;
    }

    // Write encoding output buffer to output file
    write_file(outfile, enc_out.buf, enc_out.buflen);
    
    show_info("Encoding %s => %s succeed: inlen=%-8u outlen=%-7u frame=%s\n", 
        infile.c_str(), outfile.c_str(), enc_in.buflen, enc_out.buflen, enc_out.frame == VPU_I_FRAME ? "I" : "P");
}



void encode(const string& infile, FileType intype, const string& outfile, FileType outtype, 
              size_t width, size_t height, uint16_t gopsize, uint32_t bitrate) {
    // Allocate input/output buffers.
    void*  inbuf = NULL;
    void*  outbuf = NULL;
    size_t inlen = MAX_BUFFER_SIZE;
    size_t outlen = MAX_BUFFER_SIZE;
    alloc_buffer(&inbuf, &outbuf, NULL);
    
    // Open encoder
    vpu_enc_config enc_config;
    enc_config.encfmt = outtype == JPEG_FILE ? VPU_FMT_JPEG : VPU_FMT_H264;
    enc_config.pixfmt = VPU_YUV420P;
    enc_config.width = width;
    enc_config.height = height;
    enc_config.gop_size = gopsize;
    enc_config.quality = bitrate;
    void* enc_handle = vpu_enc_open(&enc_config);
    if (!enc_handle) {
        show_error_exit("vpu_enc_open failed: %s\n", vpu_error());
    }

    // If infile is a dir
    if (is_dir(infile)) {
        // outfile must be a dir, if not existed, create it
        create_dir(outfile);
        vector<string> files = dir_files(infile);
        for (int i=0; i < files.size(); i++) {
            string inpath = files[i];
            string outpath = outfile + "/" + remove_suffix(base_name(files[i])) + (outtype == JPEG_FILE ? ".jpg" : ".264");
            
            // Encoding one frame
            encode_one_frame(enc_handle, inpath, inbuf, inlen, outpath, outbuf, outlen);  
        }
    } else {
            // Encoding one frame
            encode_one_frame(enc_handle, infile, inbuf, inlen, outfile, outbuf, outlen);      
    }
       
    // Close encoder
    if (vpu_enc_close(enc_handle) == -1) {
        show_error("vpu_enc_close failed: %s\n", vpu_error());
    }

    // Free input/output buffers.
    free_buffer(inbuf, outbuf, NULL);
}

void decode_one_frame(void* dec_handle, const string& infile, void* inbuf, size_t inlen, 
                            const string& outfile, void* outbuf, size_t outlen, size_t& width, size_t& height) {
    // Fill input buffer by reading input file
    read_file(infile, inbuf, inlen);

    // Decode one frame
    vpu_dec_in dec_in;
    dec_in.buf = (char*)inbuf;
    dec_in.buflen = inlen;
    vpu_dec_out dec_out;
    dec_out.buf = (char*)outbuf;
    dec_out.buflen = outlen;
    if (vpu_decode(dec_handle, &dec_in, &dec_out) == -1) {
        show_error_exit("Decoding %s => %s failed: %s\n", infile.c_str(), outfile.c_str(), vpu_error());
    }

    // Save the width and height
    width = dec_out.width;
    height = dec_out.height;

    // Write encoding output buffer to output file
    write_file(outfile, dec_out.buf, dec_out.buflen);

    // Decdoing end
    show_info("Decoding %s => %s succeed: inlen=%-7u outlen=%-8u width=%-5u height=%-5u\n", 
        infile.c_str(), outfile.c_str(), dec_in.buflen, dec_out.buflen, dec_out.width, dec_out.height);


}

void decode(const string& infile, FileType intype, const string& outfile, FileType outtype, size_t& width, size_t& height) {
    
    // Allocate input/output buffers.
    void*  inbuf = NULL;
    void*  outbuf = NULL;
    size_t inlen = MAX_BUFFER_SIZE;
    size_t outlen = MAX_BUFFER_SIZE;
    alloc_buffer(&inbuf, &outbuf, NULL);

    
    // Open decoder
    vpu_dec_config dec_config;
    dec_config.decfmt = intype == JPEG_FILE ? VPU_FMT_JPEG : VPU_FMT_H264;;
    void* dec_handle = vpu_dec_open(&dec_config);
    if (!dec_handle) {
        show_error_exit("vpu_dec_open failed: %s\n", vpu_error());
    }

    
    // If infile is a dir
    if (is_dir(infile)) {
        // outfile must be a dir, if not existed, create it
        create_dir(outfile);
        vector<string> files = dir_files(infile);
        for (int i=0; i < files.size(); i++) {
            string inpath = files[i];
            string outpath = outfile + "/" + remove_suffix(base_name(files[i]))  + ".yuv";

            // Encoding one frame
            decode_one_frame(dec_handle, inpath, inbuf, inlen, outpath, outbuf, outlen, width, height);
        }
    } else {
        // Encoding one frame
        decode_one_frame(dec_handle, infile, inbuf, inlen, outfile, outbuf, outlen, width, height);
    }

    
    // Close decoder
    if (vpu_dec_close(dec_handle) == -1) {
        show_error("vpu_dec_open failed: %s\n", vpu_error());
    }
    
    // Free input/output buffers.    
    free_buffer(inbuf, outbuf, NULL);
}


void transcode(const string& infile, FileType intype, const string& outfile, FileType outtype, uint16_t gopsize, uint32_t bitrate) {
    // Init tmp file name and type
    string tmpfile = "vpu_codec_tmp.yuv";
    FileType tmptype = YUV_FILE;
    size_t width = 0; 
    size_t height = 0;

    // Decoding
    decode(infile, intype, tmpfile, tmptype, width, height);

    // Encoding
    encode(tmpfile, tmptype, outfile, outtype, width, height, gopsize, bitrate);
}


//
// NOTES: alloc_file_buffer() and transcode_profiling() are only used for Performance Profiling
//
void alloc_file_buffer(const string& file, void*& outbuf, size_t& outlen) {
    // Open file
    FILE* fp = fopen(file.c_str(), "rb");
    if (!fp) {
        show_error_exit("Can't open %s\n", file.c_str());
    }

    // Get file length
    fseek(fp, 0L, SEEK_END);
    long buflen = ftell(fp);
    if (buflen <= 0) {
        show_error_exit("File %s can't be empty\n", file.c_str());        
    }
    fseek(fp, 0L, SEEK_SET);

    // Allocate buffer
    void* buf = malloc(buflen);
    if (!buf) {
       show_error_exit("malloc failed\n"); 
    }

    // Fill buffer
    long read = fread(buf, 1, buflen, fp);
    if (buflen != read) {
        show_error_exit("Only read %ld bytes from %s (required=%lu bytes)\n",  read, file.c_str(), buflen);
    }

    // Close file
    fclose(fp); 

    // Save allocated buffer and length
    outbuf = buf;
    outlen = buflen;
}

void transcode_profiling(const string& infile, FileType intype, FileType outtype, 
                            size_t width, size_t height, uint16_t gopsize, uint32_t bitrate, int repreat) {

    // Open decoder
    vpu_dec_config dec_config;
    dec_config.decfmt = intype == JPEG_FILE ? VPU_FMT_JPEG : VPU_FMT_H264;;
    void* dec_handle = vpu_dec_open(&dec_config);
    if (!dec_handle) {
        show_error_exit("vpu_dec_open failed: %s\n", vpu_error());
    }
    
    // Open encoder
    vpu_enc_config enc_config;
    enc_config.encfmt = outtype == JPEG_FILE ? VPU_FMT_JPEG : VPU_FMT_H264;
    enc_config.pixfmt = VPU_YUV420P;
    enc_config.width = width;
    enc_config.height = height;
    enc_config.gop_size = gopsize;
    enc_config.quality = bitrate;
    void* enc_handle = vpu_enc_open(&enc_config);
    if (!enc_handle) {
        show_error_exit("vpu_enc_open failed: %s\n", vpu_error());
    }

    // Prepare input buffers
    vector< pair<void*, size_t> > inbufs;
    vector<string> infiles;
    if (is_dir(infile)) {
        // If infile is a directory
        infiles = dir_files(infile);

    } else {
        // infile is a file
        infiles.push_back(infile);  
    }
    for (int i=0; i < infiles.size(); i++) {
        pair<void*, size_t> inbuf;
        alloc_file_buffer(infiles[i], inbuf.first, inbuf.second);
        inbufs.push_back(inbuf);
    }

    // Allocate a tmp buffer for saving decoding output, and encoding output.
    void* decbuf = malloc(MAX_BUFFER_SIZE);
    void* encbuf = malloc(MAX_BUFFER_SIZE);
    if (!decbuf || !encbuf) {
       show_error_exit("malloc failed\n"); 
    }

    // Loop profiling
    TimeTrack tt;
    vpu_dec_in dec_in;
    vpu_dec_out dec_out;
    vpu_enc_in enc_in;
    vpu_enc_out enc_out;
    for (int n=0; n < repreat; n++) {
        for (int i=0; i < inbufs.size(); i++) {
            uint64_t start = tt.now();
            
            // Decode one frame
            tt.mark();
            dec_in.buf = (char*)(inbufs[i].first);
            dec_in.buflen = inbufs[i].second;
            dec_out.buf = (char*)decbuf;
            dec_out.buflen = MAX_BUFFER_SIZE;
            if (vpu_decode(dec_handle, &dec_in, &dec_out) == -1) {
                show_error("Decoding %s => YUV failed: %s\n", infiles[i].c_str(), vpu_error());

                //
                // NOTES: Some times, VPU decoding error occurs, we can restart the decoder and try it again. 
                //

                // Close decoder
                if (vpu_dec_close(dec_handle) == -1) {
                    show_error_exit("vpu_dec_close failed: %s\n", vpu_error());
                }
                // Open decoder
                dec_handle = vpu_dec_open(&dec_config);
                if (!dec_handle) {
                    show_error_exit("vpu_dec_open failed: %s\n", vpu_error());
                }
                // Decode it again
                if (vpu_decode(dec_handle, &dec_in, &dec_out) == -1) {
                    show_error_exit("Decoding %s => YUV failed: %s\n", infiles[i].c_str(), vpu_error());
                }
            }
            show_info("Decoding %s => YUV succeed: inlen=%-7u outlen=%-8u width=%-5u height=%-5u duration=%-5llu (loop=%-5d index=%d)\n", 
                infiles[i].c_str(), dec_in.buflen, dec_out.buflen, dec_out.width, dec_out.height, tt.duration(), n, i);
            
            // Encode one frame
            tt.mark();
            enc_in.buf = dec_out.buf;
            enc_in.buflen = dec_out.buflen;
            enc_out.buf = (char*)encbuf;
            enc_out.buflen = MAX_BUFFER_SIZE;
            if (vpu_encode(enc_handle, &enc_in, &enc_out) == -1) {
                show_error_exit("Encoding YUV => %s failed: %s\n", 
                    (outtype == JPEG_FILE ? "JPEG" : "H.264"), vpu_error());
            }
            show_info("Encoding YUV => %s succeed: inlen=%-8u outlen=%-7u frame=%s duration=%-5llu\n", 
                        (outtype == JPEG_FILE ? "JPEG" : "H.264"),  
                        enc_in.buflen, enc_out.buflen, (enc_out.frame == VPU_I_FRAME ? "I" : "P"),
                        tt.duration());

            uint64_t end = tt.now();
            if (end - start < 50) {
                usleep((50+start-end)*1000);
            }
        }
    }

    // Close encoder
    if (vpu_enc_close(enc_handle) == -1) {
        show_error("vpu_dec_open failed: %s\n", vpu_error());
    }
    
    // Close decoder
    if (vpu_dec_close(dec_handle) == -1) {
        show_error("vpu_dec_open failed: %s\n", vpu_error());
    }

}

int main(int argc, char** argv) {
    if (argc == 1 || (argc == 2 && string(argv[1]) == "-h")) {
        show_usage_exit();
    }

    // Parse arguments
    string infile;
    string outfile;
    size_t width = 0;
    size_t height = 0;
    uint16_t gopsize = 0;
    uint32_t bitrate = 0;
    int loopcount = 0;
    int opt = -1;
    while ((opt = getopt(argc, argv, "i:o:w:h:g:b:p:")) != -1) {
        switch (opt) {
        case 'i':
            infile = optarg;
            break;
        case 'o':
            outfile = optarg;
            break;
        case 'w':
            width = atoi(optarg);
            break;
        case 'h':
            height = atoi(optarg);
            break;      
        case 'g':
            gopsize = atoi(optarg);
            break;
        case 'b':
            bitrate = atoi(optarg);
            break; 
        case 'p':
            loopcount = atoi(optarg);
            break;     
        default:
            show_usage_exit();
        }
    }

    // Check arguments
    if (infile.empty() || outfile.empty()) {
        show_error("The <input-file> and/or <output-file> can't be empty\n");
        show_usage_exit();
    }
    FileType intype = get_file_type(infile);
    if (intype == UNKNOW_FILE) {
        show_error("The type of '%s' is unknown. The file name suffix should be '.yuv' or '.jpeg'\n", infile.c_str());
        show_usage_exit();

    }
    FileType outtype = get_file_type(outfile);
    if (outtype == UNKNOW_FILE) {
        show_error("The type of '%s' is unknown. The file name suffix should be '.yuv' or '.jpeg'\n", outfile.c_str());
        show_usage_exit();
    }
    if (intype == outtype) {
        show_error("The type of '%s' is same as '%s'\n", infile.c_str(), outfile.c_str());
        show_usage_exit();
    }
    if (intype == YUV_FILE && (height <= 0 || height <= 0)) {
        show_error("The <picture-width> and/or <picture-height> is invalid\n");
        show_usage_exit();
    }
    if (loopcount > 0 && (height <= 0 || height <= 0 || intype == YUV_FILE || outtype == YUV_FILE)) {
        show_error("Transcoding profiling arguments is invalid\n");
        show_usage_exit();        
    }
    
    // Initialize vpu
    if (vpu_init() == -1) {
         show_error_exit("vpu_init() failed: %s\n", vpu_error());
    }

    WorkType work = (intype == YUV_FILE) ? ENCODING : outtype == YUV_FILE ? DECODING : TRANSCODING;
    if (work == ENCODING) {
        // Do encoding
        encode(infile, intype, outfile, outtype, width, height, gopsize, bitrate);
    } else if (work == DECODING) {
        // Do decoding
        decode(infile, intype, outfile, outtype, width, height);
    } else {
        if (loopcount == 0) {
            // Do transcoding
            transcode(infile, intype, outfile, outtype, gopsize, bitrate);
        } else {
            // Do transcoding profiling
            transcode_profiling(infile, intype, outtype, width, height, gopsize, bitrate, loopcount);
        }
    }

    // Deinitialize vpu
    if (vpu_deinit() == -1) {
        show_error_exit("vpu_deinit failed: %s\n", vpu_error());
    }
    
    return 0;
}
