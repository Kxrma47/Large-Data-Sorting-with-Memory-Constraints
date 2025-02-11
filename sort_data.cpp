//This is to show what the code does
///* Returns the size of the file, or exits with an error if unsuccessful. */
//static size_t getFileSize(const std::string &filename);
//
///* Structure for storing pointers and sizes when using mmap. */
//struct MappedRegion {
//    void* ptr;
//    size_t mappingSize;
//    void* mmappedAddr;
//};
//
///* Aligns the offset correctly to the page boundary (especially for macOS)
//   to prevent mmap errors. Returns a structure with the required pointers. */
//static MappedRegion mmapWithPageAlign(
//    size_t offsetInFile,
//    size_t mapSizeBytes,
//    int protFlags,
//    int mapFlags,
//    int fd
//);
//
///* Generates a binary file with 64-bit integers (random).
//   If sorted=true, the numbers are sorted before writing to get pre-sorted data. */
//static void generateFile(const std::string &filename, size_t count, bool sorted=false);
//
///* Checks if the file (containing 64-bit integers) is sorted in ascending order.
//   Reads data in small chunks and compares pairs. */
//static void checkFile(const std::string &filename);
//
///* Sorts a chunk of size count starting from offsetElems.
//   If inPlace=false, copies the result to another file. Uses mmap. */
//static void sortChunkAndWrite(
//    int inFD,
//    int outFD,
//    size_t offElems,
//    size_t count,
//    size_t inOffBytes,
//    size_t outOffBytes,
//    size_t mapSizeBytes,
//    bool inPlace
//);
//
///* Information about a run (partially sorted fragment):
//   offset (in elements), length (in elements). */
//struct RunInfo {
//    size_t offset;
//    size_t length;
//};
//
///* Splits the entire input file into chunks, sorts each chunk,
//   and writes it to outFD as a separate run. Returns a list of RunInfo. */
//static std::vector<RunInfo> createInitialRuns(
//    int inFD,
//    int outFD,
//    size_t totalElems,
//    size_t chunkBytes
//);
//
///* Heap item for k-way merging:
//   value - the current value, runIdx - the index of the run from which it was taken. */
//struct HeapItem {
//    long long value;
//    int runIdx;
//};
//static bool operator>(const HeapItem &a,const HeapItem &b);
//
///* Structure for tracking the state of a run during multi-way merging:
//   fileOffset - start of the run in elements,
//   length - total elements in the run,
//   consumed - number of elements already processed,
//   buffer - buffer for preloading,
//   bufPos - pointer to the current element,
//   done - flag indicating if the run is exhausted. */
//struct MergeRunState {
//    size_t fileOffset;
//    size_t length;
//    size_t consumed;
//    std::vector<long long> buffer;
//    size_t bufPos;
//    bool done;
//};
//
///* does k-way merging of a list of runs from inFD into outFD, starting
//   at outOffset, using no more than memBytes of memory (distributed among buffers). */
//static void multiWayMerge(
//    int inFD,
//    int outFD,
//    const std::vector<RunInfo> &runs,
//    size_t outOffset,
//    size_t memBytes
//);
//
///* Performs one pass of multi-way merging: merges groups of maxK runs,
//   creating fewer runs. Returns the new runs. */
//static std::vector<RunInfo> multiWayMergePass(
//    int inFD,
//    int outFD,
//    const std::vector<RunInfo> &runs,
//    size_t memBytes,
//    size_t maxK
//);
//
///* Main sorting function:
//   1) Opens the file and checks its size
//   2) Quickly checks if the file is already sorted
//   3) Creates initial runs (createInitialRuns)
//   4) Multi-pass k-way merging (multiWayMergePass) until one run remains
//   5) If the final sorted data is in a temporary file, copies it back to the main file */
//static void externalSort(const std::string &filename, size_t memoryLimitMB);
//
///* Entry point:
//   --gen <file> <count> [sorted]  => Generate a file with random numbers
//   --check <file>                 => Check if the file is sorted
//   <file> [limitMB]               => Sort the file with an optional memory limit */
//int main(int argc,char* argv[]);
//




#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static size_t getFileSize(const std::string &filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) != 0) {
        perror("stat failed");
        exit(1);
    }
    return (size_t)st.st_size;
}

struct MappedRegion {
    void* ptr;
    size_t mappingSize;
    void* mmappedAddr;
};

static MappedRegion mmapWithPageAlign(size_t offsetInFile, size_t mapSizeBytes,
                                      int protFlags, int mapFlags, int fd) {
    MappedRegion result;
    result.ptr = nullptr;
    result.mappingSize = 0;
    result.mmappedAddr = nullptr;
    if (mapSizeBytes == 0) return result;
    long ps = sysconf(_SC_PAGE_SIZE);
    if (ps < 1) ps = 4096;
    off_t alignOffset = (offsetInFile / ps) * ps;
    size_t diff = offsetInFile - alignOffset;
    size_t fullSize = mapSizeBytes + diff;
    void* addr = mmap(nullptr, fullSize, protFlags, mapFlags, fd, alignOffset);
    if (addr == MAP_FAILED) return result;
    result.mmappedAddr = addr;
    result.mappingSize = fullSize;
    result.ptr = static_cast<char*>(addr) + diff;
    return result;
}

static void generateFile(const std::string &filename, size_t count, bool sorted) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<long long> dist(
        std::numeric_limits<long long>::min(),
        std::numeric_limits<long long>::max()
    );
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        std::cerr << "Cannot open file for writing: " << filename << "\n";
        exit(1);
    }
    std::vector<long long> data(count);
    for (size_t i = 0; i < count; i++) data[i] = dist(gen);
    if (sorted) std::sort(data.begin(), data.end());
    ofs.write(reinterpret_cast<char*>(data.data()), count*sizeof(long long));
    ofs.close();
    std::cout << "Generated " << count << " 64-bit integers into " << filename
              << (sorted ? " (sorted)." : ".") << "\n";
}

static void checkFile(const std::string &filename) {
    size_t fs = getFileSize(filename);
    if (fs % sizeof(long long) != 0) {
        std::cerr << "File size not multiple of 8 bytes => not 64-bit ints.\n";
        exit(1);
    }
    size_t total = fs / sizeof(long long);
    if (total < 2) {
        std::cout << "File has 0 or 1 element => trivially sorted.\n";
        return;
    }
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        std::cerr << "Cannot open file for reading: " << filename << "\n";
        exit(1);
    }
    const size_t BUF = 1 << 15;
    std::vector<long long> buf(BUF);
    long long prev;
    bool first=true;
    size_t readCount=0;
    while (readCount < total) {
        size_t toRead = std::min(BUF, total - readCount);
        ifs.read(reinterpret_cast<char*>(buf.data()), toRead*sizeof(long long));
        if (!ifs) {
            std::cerr << "Error reading file.\n";
            exit(1);
        }
        for (size_t i=0; i<toRead; i++) {
            if (!first) {
                if (buf[i] < prev) {
                    std::cerr << "File is NOT sorted (found " << buf[i]
                              << " after " << prev << ").\n";
                    return;
                }
            }
            prev=buf[i];
            first=false;
        }
        readCount += toRead;
    }
    std::cout << "File is sorted ascending.\n";
}

static void sortChunkAndWrite(int inFD,int outFD,size_t offElems,size_t count,
                              size_t inOffBytes,size_t outOffBytes,size_t mapSizeBytes,
                              bool inPlace) {
    auto region = mmapWithPageAlign(inOffBytes,mapSizeBytes,PROT_READ|PROT_WRITE,MAP_SHARED,inFD);
    if (region.mmappedAddr == MAP_FAILED && mapSizeBytes>0) {
        perror("sortChunk: mmap failed");
        exit(1);
    }
    if (!region.ptr && mapSizeBytes>0) {
        std::cerr<<"sortChunk: got null region\n";
        exit(1);
    }
    long long* ptr = (long long*)region.ptr;
    std::sort(ptr, ptr+count);
    if (!inPlace) {
        auto outRegion = mmapWithPageAlign(outOffBytes,mapSizeBytes,PROT_READ|PROT_WRITE,MAP_SHARED,outFD);
        if (outRegion.mmappedAddr == MAP_FAILED && mapSizeBytes>0) {
            perror("sortChunk: mmap out failed");
            if (region.mmappedAddr != MAP_FAILED && region.mappingSize>0) {
                munmap(region.mmappedAddr, region.mappingSize);
            }
            exit(1);
        }
        if (outRegion.ptr && mapSizeBytes>0) {
            std::memcpy(outRegion.ptr,region.ptr,mapSizeBytes);
        }
        if (outRegion.mmappedAddr != MAP_FAILED && outRegion.mappingSize>0) {
            munmap(outRegion.mmappedAddr,outRegion.mappingSize);
        }
    }
    if (region.mmappedAddr != MAP_FAILED && region.mappingSize>0) {
        munmap(region.mmappedAddr,region.mappingSize);
    }
}

struct RunInfo {
    size_t offset;
    size_t length;
};

static std::vector<RunInfo> createInitialRuns(int inFD,int outFD,size_t totalElems,size_t chunkBytes) {
    std::vector<RunInfo> runs;
    size_t off=0;
    size_t chunkElems = chunkBytes/sizeof(long long);
    if (chunkElems<1) chunkElems=1;
    while (off<totalElems) {
        size_t c = std::min(chunkElems,totalElems-off);
        size_t inOffB = off*sizeof(long long);
        size_t mapSize = c*sizeof(long long);
        sortChunkAndWrite(inFD,outFD,off,c,inOffB,inOffB,mapSize,false);
        RunInfo r; r.offset=off; r.length=c;
        runs.push_back(r);
        off+=c;
    }
    return runs;
}

struct HeapItem {
    long long value;
    int runIdx;
};
static bool operator>(const HeapItem &a,const HeapItem &b) {
    return a.value > b.value;
}

struct MergeRunState {
    size_t fileOffset;
    size_t length;
    size_t consumed;
    std::vector<long long> buffer;
    size_t bufPos;
    bool done;
};

static void multiWayMerge(int inFD,int outFD,const std::vector<RunInfo> &runs,
                          size_t outOffset,size_t memBytes) {
    if (runs.empty()) return;
    size_t totalLen=0;
    for (auto &r : runs) totalLen += r.length;
    size_t k = runs.size();
    if (k==1) {
        size_t off = runs[0].offset;
        size_t ln = runs[0].length;
        size_t totalB = ln*sizeof(long long);
        size_t stepB = memBytes>0?memBytes:(1<<20);
        size_t done=0;
        while (done<totalB) {
            size_t s=totalB-done; if (s>stepB) s=stepB;
            auto inMap = mmapWithPageAlign(off*sizeof(long long)+done,s,PROT_READ,MAP_SHARED,inFD);
            if (inMap.mmappedAddr==MAP_FAILED && s>0) {
                perror("multiWayMerge single-run inMap fail");
                exit(1);
            }
            auto outMap= mmapWithPageAlign(outOffset*sizeof(long long)+done,s,
                                           PROT_READ|PROT_WRITE,MAP_SHARED,outFD);
            if (outMap.mmappedAddr==MAP_FAILED && s>0) {
                perror("multiWayMerge single-run outMap fail");
                if (inMap.mmappedAddr!=MAP_FAILED && inMap.mappingSize>0) {
                    munmap(inMap.mmappedAddr,inMap.mappingSize);
                }
                exit(1);
            }
            if (inMap.ptr && outMap.ptr && s>0) {
                std::memcpy(outMap.ptr,inMap.ptr,s);
            }
            if (inMap.mmappedAddr!=MAP_FAILED && inMap.mappingSize>0) {
                munmap(inMap.mmappedAddr,inMap.mappingSize);
            }
            if (outMap.mmappedAddr!=MAP_FAILED && outMap.mappingSize>0) {
                munmap(outMap.mmappedAddr,outMap.mappingSize);
            }
            done+=s;
        }
        return;
    }
    size_t memForEach = memBytes/(k+1);
    memForEach=(memForEach/8)*8;
    if (memForEach<8) memForEach=8;
    std::vector<MergeRunState> st(k);
    size_t outBufBytes=memForEach;
    size_t outBufCount=outBufBytes/sizeof(long long);
    if (outBufCount<1) outBufCount=1;
    std::vector<long long> outBuf(outBufCount);
    size_t outPos=0;
    size_t curOut=outOffset;
    for (size_t i=0;i<k;i++){
        st[i].fileOffset=runs[i].offset;
        st[i].length=runs[i].length;
        st[i].consumed=0;
        st[i].done=false;
        st[i].bufPos=0;
        size_t eachCount= memForEach/sizeof(long long);
        if (eachCount<1) eachCount=1;
        st[i].buffer.resize(eachCount);
    }
    std::priority_queue<HeapItem,std::vector<HeapItem>,std::greater<HeapItem>> minHeap;
    auto refill=[&](int idx){
        MergeRunState &rs= st[idx];
        rs.bufPos=0;
        size_t left= rs.length - rs.consumed;
        if (left==0) { rs.done=true; return; }
        size_t space= rs.buffer.size();
        if (space>left) space=left;
        size_t startB= (rs.fileOffset+rs.consumed)*sizeof(long long);
        size_t mapB= space*sizeof(long long);
        auto inMap= mmapWithPageAlign(startB,mapB,PROT_READ,MAP_SHARED,inFD);
        if (inMap.mmappedAddr==MAP_FAILED && mapB>0) {
            perror("multiWayMerge: refill mmap failed");
            exit(1);
        }
        if (inMap.ptr && mapB>0) {
            std::memcpy(rs.buffer.data(), inMap.ptr, mapB);
        }
        if (inMap.mmappedAddr!=MAP_FAILED && inMap.mappingSize>0) {
            munmap(inMap.mmappedAddr,inMap.mappingSize);
        }
        rs.consumed+=space;
    };
    for (size_t i=0;i<k;i++){
        refill(i);
        if (!st[i].done) {
            HeapItem itm; itm.value= st[i].buffer[0]; itm.runIdx=(int)i;
            minHeap.push(itm);
            st[i].bufPos=1;
        }
    }
    auto flushOut=[&](){
        if (outPos==0) return;
        size_t mapB= outPos*sizeof(long long);
        size_t startB= curOut*sizeof(long long);
        auto outMap= mmapWithPageAlign(startB,mapB,PROT_READ|PROT_WRITE,MAP_SHARED,outFD);
        if (outMap.mmappedAddr==MAP_FAILED && mapB>0) {
            perror("multiWayMerge: flushOutBuffer mmap failed");
            exit(1);
        }
        if (outMap.ptr && mapB>0) {
            std::memcpy(outMap.ptr,outBuf.data(),mapB);
        }
        if (outMap.mmappedAddr!=MAP_FAILED && outMap.mappingSize>0) {
            munmap(outMap.mmappedAddr,outMap.mappingSize);
        }
        curOut+= outPos;
        outPos=0;
    };
    while (!minHeap.empty()) {
        auto top= minHeap.top();
        minHeap.pop();
        outBuf[outPos++]= top.value;
        if (outPos== outBufCount) flushOut();
        int ridx= top.runIdx;
        MergeRunState &rs= st[ridx];
        if (!rs.done) {
            if (rs.bufPos>= rs.buffer.size()) {
                refill(ridx);
                rs.bufPos=0;
                if (rs.done) continue;
            }
            HeapItem nxt; nxt.value= rs.buffer[rs.bufPos]; nxt.runIdx=ridx;
            minHeap.push(nxt);
            rs.bufPos++;
        }
    }
    flushOut();
}

static std::vector<RunInfo> multiWayMergePass(int inFD,int outFD,const std::vector<RunInfo> &runs,
                                              size_t memBytes,size_t maxK) {
    std::vector<RunInfo> newRuns;
    newRuns.reserve((runs.size()+maxK-1)/maxK);
    size_t outOff=0;
    for (size_t i=0;i<runs.size();i+=maxK) {
        size_t grp= std::min(maxK,runs.size()-i);
        std::vector<RunInfo> group; group.reserve(grp);
        size_t totalLen=0;
        for (size_t j=0;j<grp;j++){
            group.push_back(runs[i+j]);
            totalLen+= runs[i+j].length;
        }
        multiWayMerge(inFD,outFD,group,outOff,memBytes);
        RunInfo nr; nr.offset= outOff; nr.length= totalLen;
        newRuns.push_back(nr);
        outOff+= totalLen;
    }
    return newRuns;
}

static void externalSort(const std::string &filename,size_t memoryLimitMB) {
    int fd= open(filename.c_str(),O_RDWR);
    if (fd<0) {
        perror("open input file");
        exit(1);
    }
    size_t fs= getFileSize(filename);
    if (fs% sizeof(long long)!=0) {
        std::cerr<<"File size not multiple of 8 => not 64-bit ints.\n";
        close(fd);
        exit(1);
    }
    size_t total= fs/sizeof(long long);
    if (total<=1) {
        std::cout<<"File has <= 1 element, already sorted.\n";
        close(fd);
        return;
    }
    {
        std::cout<<"Checking if the file is already sorted...\n";
        std::ifstream f(filename,std::ios::binary);
        if (f) {
            const size_t SAMP=1000;
            bool sorted=true;
            long long prev=0,cur=0;
            size_t step= total/SAMP;
            if (step<1) step=1;
            bool first=true;
            size_t idx=0;
            while (idx<total) {
                f.seekg(idx*sizeof(long long),std::ios::beg);
                if (!f.read(reinterpret_cast<char*>(&cur),sizeof(cur))) break;
                if (!first) {
                    if (cur<prev) {
                        sorted=false;
                        break;
                    }
                }
                first=false;
                prev=cur;
                idx+= step;
            }
            f.close();
            if (sorted) {
                std::cout<<"✅ File is already sorted. Skipping sorting.\n";
                close(fd);
                return;
            }
        }
    }
    size_t usedMem=0;
    if (memoryLimitMB==0) {
        usedMem= fs/10;
        long ps= sysconf(_SC_PAGE_SIZE);
        if (ps<1) ps=4096;
        if (usedMem< (size_t)ps) usedMem= ps;
    } else {
        usedMem= memoryLimitMB*1024ULL*1024ULL;
    }
    std::cout<<"File has "<<total<<" elements ("<<fs<<" bytes). Using up to "
             << (usedMem/(1024.0*1024.0)) <<" MB of mmap.\n";
    std::cout<<"🔹 Starting external multi-way mergesort...\n";
    std::string tempName= filename+".tmp_sort";
    int fdTemp= open(tempName.c_str(),O_RDWR|O_CREAT|O_TRUNC,0666);
    if (fdTemp<0) {
        perror("open temp file");
        close(fd);
        exit(1);
    }
    if (ftruncate(fdTemp,fs)!=0) {
        perror("ftruncate temp");
        close(fdTemp);
        close(fd);
        exit(1);
    }
    size_t chunkBytes= usedMem;
    if (chunkBytes<8) chunkBytes=8;
    auto runs= createInitialRuns(fd,fdTemp,total,chunkBytes);
    bool flip=true;
    std::vector<RunInfo> currentRuns= runs;
    int inFD= fdTemp;
    int outFD= fd;
    auto computeMaxK=[&](size_t memB){
        const size_t minBuf=16UL*1024UL;
        if (memB< minBuf*2) return (size_t)2;
        size_t g=(memB/(minBuf))-1;
        if (g<2) g=2;
        if (g>1024) g=1024;
        return g;
    };
    size_t maxK= computeMaxK(usedMem);
    while (currentRuns.size()>1) {
        if (ftruncate(outFD,fs)!=0) {
            perror("ftruncate outFD");
            close(fd);
            close(fdTemp);
            exit(1);
        }
        auto newRuns= multiWayMergePass(inFD,outFD,currentRuns,usedMem,maxK);
        currentRuns= newRuns;
        int tmp= inFD; inFD= outFD; outFD= tmp;
        flip=!flip;
    }
    if (!flip) {
        lseek(fdTemp,0,SEEK_SET);
        lseek(fd,0,SEEK_SET);
        const size_t BUFSZ=1<<20;
        std::vector<char> buf(BUFSZ);
        size_t left=fs;
        while (left>0) {
            size_t s=(left<BUFSZ?left:BUFSZ);
            ssize_t rd=read(fdTemp,buf.data(),s);
            if (rd<0) {
                perror("read fdTemp final copy");
                exit(1);
            }
            if (rd==0) break;
            ssize_t wr=write(fd,buf.data(),rd);
            if (wr<0) {
                perror("write final to original");
                exit(1);
            }
            left-=wr;
        }
    }
    close(fd);
    close(fdTemp);
    remove(tempName.c_str());
    std::cout<<"Sorting complete.\n";
}

int main(int argc,char* argv[]){
    if (argc<2){
        std::cerr<<"Usage:\n"
                 <<argv[0]<<" --gen <filename> <count> [sorted]\n"
                 <<argv[0]<<" --check <filename>\n"
                 <<argv[0]<<" <filename> [limitMB]\n";
        return 1;
    }
    if (std::string(argv[1])=="--gen"){
        if (argc<4){
            std::cerr<<"Usage: "<<argv[0]<<" --gen <filename> <count> [sorted]\n";
            return 1;
        }
        std::string fn= argv[2];
        size_t cnt=0;
        try{cnt= std::stoull(argv[3]);}catch(...){
            std::cerr<<"Invalid count.\n";return 1;
        }
        bool mkSorted=false;
        if (argc>=5 && std::string(argv[4])=="sorted") mkSorted=true;
        generateFile(fn,cnt,mkSorted);
        return 0;
    }
    if (std::string(argv[1])=="--check"){
        if (argc<3){
            std::cerr<<"Usage: "<<argv[0]<<" --check <filename>\n";
            return 1;
        }
        checkFile(argv[2]);
        return 0;
    }
    std::string fn= argv[1];
    size_t limitMB=0;
    if (argc>=3){
        try {
            limitMB= std::stoull(argv[2]);
        }catch(const std::invalid_argument&){
            std::cerr<<"Error: Invalid memory limit. Please enter a valid number.\n";
            return 1;
        }catch(const std::out_of_range&){
            std::cerr<<"Error: Memory limit too large.\n";
            return 1;
        }
    }
    externalSort(fn,limitMB);
    return 0;
}
