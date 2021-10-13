#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256


void decToBinary(int n , char &c) 
{ 
   // array to store binary number 
    int binaryNum[8]; 
    c = 0;
    // counter for binary array 
    int i = 0; 
    while (n > 0) { 
          // storing remainder in binary array 
        binaryNum[i] = n % 2; 
        n = n / 2; 
        i++; 
    } 
  
    // printing binary array in reverse order 
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j]==1)
            c = c | 1u << j;
    }
 } 

// #define SYS_CALL
// ============================================================================
class fsInode {
    int fileSize;
    int block_in_use;

    int *directBlocks;
    int singleInDirect;
    int num_of_direct_blocks;
    int block_size;


    public:
    fsInode(int _block_size, int _num_of_direct_blocks) {
        fileSize = 0; 
        block_in_use = 0; 
        block_size = _block_size;
        num_of_direct_blocks = _num_of_direct_blocks;
        directBlocks = new int[num_of_direct_blocks];
		assert(directBlocks);
        for (int i = 0 ; i < num_of_direct_blocks; i++) {   
            directBlocks[i] = -1;
        }
        singleInDirect = -1;

    }
    int* getDirectBlocks(){
        return this->directBlocks;
    }


    int getSingleInDirect(){
        return this->singleInDirect;
    }


    void setSingleInDirect(int set){
        this->singleInDirect = set;
    }


    int getFileSize(){
        return this->fileSize;
    }


    void setFileSize(int size){
        this->fileSize = size;
    }

    int getBlocksInUse(){
        return this->block_in_use;
    }


    void incrementBolcksinUse(){
        this->block_in_use ++;
    }


    ~fsInode() { 
        delete [] directBlocks;
    }


};

// ============================================================================
class FileDescriptor {
    pair<string, fsInode*> file;
    bool inUse;
    bool Deleted;

    public:
    FileDescriptor(string FileName, fsInode* fsi) {
        file.first = FileName;
        file.second = fsi;
        inUse = true;
        Deleted = false;

    }

    string getFileName() {
        return file.first;
    }

    fsInode* getInode() {
        
        return file.second;

    }

    bool isDeleted(){
        return (Deleted);
    }
    bool isInUse() { 
        return (inUse); 
    }
    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }
    
    void setDeletedStat(bool stat){
        this->Deleted = stat;
    }
};
 
#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class fsDisk {
    FILE *sim_disk_fd;
 
    bool is_formated;

	// BitVector - "bit" (int) vector, indicate which block in the disk is free
	//              or not.  (i.e. if BitVector[0] == 1 , means that the 
	//             first block is occupied. 
    int BitVectorSize;
    int *BitVector;

    // Unix directories are lists of association structures, 
    // each of which contains one filename and one inode number.
    map<string, fsInode*>  MainDir ; 

    // OpenFileDescriptors --  when you open a file, 
	// the operating system creates an entry to represent that file
    // This entry number is the file descriptor. 
    vector< FileDescriptor > OpenFileDescriptors;

    int direct_enteris;
    int block_size;
    public:
    // ------------------------------------------------------------------------
    fsDisk() {
        sim_disk_fd = fopen( DISK_SIM_FILE , "r+" );
        this->is_formated = false;
        assert(sim_disk_fd);
        for (int i = 0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd );
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);

    }
	

    // ------------------------------------------------------------------------
    void listAll() {
        int i = 0;    
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: " << it->isInUse() << endl; 
            i++;
        }
        char bufy;
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fread(  &bufy , 1 , 1, sim_disk_fd );
             cout << bufy;              
        }
        cout << "'" << endl;


    }
 
    // ------------------------------------------------------------------------
    // this function  will format the disk in a way that the first parameter will be the block_size, and the seconde parameter will be the number of direcet_blocks for each file
   // also this function can not be called multiple times ( it can be called just one time each run)
    void fsFormat( int blockSize = 4, int direct_Enteris_ = 3 ) {
        if(this->is_formated == true){
            cout<< "disk is already been formatted\n";
            return;
        }
        this->block_size = blockSize;
        this->direct_enteris = direct_Enteris_;
        this->BitVectorSize = DISK_SIZE / this->block_size;
        this->BitVector = new int[BitVectorSize];
        if(this->BitVector == nullptr){
            perror("malloc failed");
            exit(1);
        }
        int i;
        for(int i = 0 ; i < this->BitVectorSize ; i++){
            this->BitVector[i] = 0;
        }
        this->is_formated = true;
    }

    // ------------------------------------------------------------------------
    // this funtion will try to create an empty  virtualFile in the disk file, if an error acure a -1 will be returned as a return value other wise the fileDiscriptor will be returned (if file with the same name already exits the function will return -1)
    int CreateFile(string fileName) {
        if(!this->is_formated){
            return -1;
        }
        if(this->MainDir.empty()){
            fsInode *toAdd = new fsInode(this->block_size, this->direct_enteris);
            if(toAdd == nullptr){
                perror("malloc faile");
                exit(EXIT_FAILURE);
            }
            FileDescriptor DtoAdd(fileName, toAdd);
            DtoAdd.setInUse(true);
            DtoAdd.setDeletedStat(false);
            this->MainDir[fileName] = toAdd;
            this->OpenFileDescriptors.push_back(DtoAdd);
            return this->OpenFileDescriptors.size() - 1;
        }
        map<string, fsInode*>::iterator it;
        it = this->MainDir.find(fileName);
        if(it != this->MainDir.end()){
            cout << "file already exsits\n";
            return -1;
        }
        fsInode *toAdd = new fsInode(this->block_size, this->direct_enteris);
        this->MainDir[fileName]= toAdd;
         FileDescriptor DtoAdd(fileName, toAdd);
            DtoAdd.setInUse(true);
            DtoAdd.setDeletedStat(false);
            this->MainDir[fileName] = toAdd;
            this->OpenFileDescriptors.push_back(DtoAdd);
            return this->OpenFileDescriptors.size() - 1;

    }

    // ------------------------------------------------------------------------
    // this function will try to open a virtualFile if an error acure -1 will be returned as a return value otherwise the function will return the FileDiscreptor of the fileName (if file is already opened a -1 will be returned)
    int OpenFile(string fileName) {
        int i;
        int size = this->OpenFileDescriptors.size();
         map<string, fsInode*>::iterator it;
         it = this->MainDir.find(fileName);
         if(it == this->MainDir.end()){
             perror("file dose not exist\n");
             return -1;
         }
        for(i = 0 ; i < size ; i++){
            if(this->OpenFileDescriptors[i].getFileName() == fileName && this->OpenFileDescriptors[i].isInUse()){
                fprintf(stderr, "file already opened\n");
                return i;
            }
            else if (this->OpenFileDescriptors[i].getFileName() == fileName && !this->OpenFileDescriptors[i].isInUse() && !this->OpenFileDescriptors[i].isDeleted()){
                this->OpenFileDescriptors[i].setInUse(true);
                return i;
            }
        }
    }  

    // ------------------------------------------------------------------------
    // this fucntion will try to close a virtualFile if an error acure the function will return a "-1" otherwise the function will return the name of the file with the given FD number
    string CloseFile(int fd) {
        int size = this->OpenFileDescriptors.size();
        if( fd >= 0 && fd < size){
            if(this->OpenFileDescriptors[fd].isInUse()){
                this->OpenFileDescriptors[fd].setInUse(false);
                return this->OpenFileDescriptors[fd].getFileName();
            }
            else{
                cout << "file already closed\n";
                return "-1";
            }
        }
        return "-1";
    }

    // ------------------------------------------------------------------------
    //this function will try to write on a virtualFile in case of error (file does not exit , there is no enough space in file or in disk , trying to wirte more than the max allowed size)
    //the fucntion will return -1, otherwise the fucntion will return the number of written bits in success
    int WriteToFile(int fd, char *buf, int len ) {
        int i;
        int j = 0;
        int k =0;
        int written = 0;
        int maxFileSize = this->block_size *(this->direct_enteris + this->block_size);
        int size = this->OpenFileDescriptors.size();
        if(fd < 0 || fd >= size){
            fprintf(stderr, "file dose not exists\n");
            return -1;
        }
        if(!this->OpenFileDescriptors[fd].isInUse()){
            fprintf(stderr, "can not write to a closed / deleted file file\n");
            return -1;
        }
        if(this->OpenFileDescriptors[fd].getInode()->getFileSize() == maxFileSize){
            fprintf(stderr, "file is full\n");
            return -1;
        }
        if(this->OpenFileDescriptors[fd].getInode()->getFileSize() + len > maxFileSize){
            fprintf(stderr, "there is no enough space in the file\n");
            return -1;
        }  
        fsInode* toUse = this->OpenFileDescriptors[fd].getInode();
        if(toUse == nullptr){
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        int freeSpaceInBlock = toUse->getBlocksInUse() * block_size - toUse->getFileSize();
        double blocksNeeded = (double)(len - freeSpaceInBlock) / block_size;
        if(blocksNeeded < 0){
            blocksNeeded *= -1;
        }
        if(blocksNeeded > howManyEmptyBlocks()){
            fprintf(stderr, "there is no empty space in the disk\n");
            return -1;
        }
        int currentBlock = -1;
        if(blocksNeeded - (int)blocksNeeded > 0){
            blocksNeeded = (int)(blocksNeeded + 1);
        }
        if(toUse->getBlocksInUse() <= direct_enteris){
            if(freeSpaceInBlock != 0){
            currentBlock = toUse->getDirectBlocks()[toUse->getBlocksInUse() - 1];
            }
            if(toUse->getBlocksInUse() == 0){
                currentBlock = findEmptyBlock();
                toUse->getDirectBlocks()[0] = currentBlock;
                toUse->incrementBolcksinUse();
                freeSpaceInBlock = block_size;
            }
        }
        else{
            if(toUse->getSingleInDirect() == -1){
                blocksNeeded += 1;
                if(blocksNeeded > howManyEmptyBlocks()){
                    fprintf(stderr, "there is no enough space in disk\n");
                    return -1;
                }
                toUse->setSingleInDirect(findEmptyBlockFromTheEnd());
                
                blocksNeeded --;
            }
            i = toUse->getBlocksInUse() - direct_enteris - 1;
            if(i < 0){
                i = 0;
            }
            char num;
            while(i < block_size){
                if(fseek(sim_disk_fd, (toUse->getSingleInDirect() * block_size) + i, SEEK_SET) != 0){
                    perror("fseek failed");
                    exit(EXIT_FAILURE);
                }
                if(fread(&num, 1, 1, sim_disk_fd) == 0){
                    perror("fread failed");
                    exit(EXIT_FAILURE);
                }
                if(num != '\0'){
                    currentBlock = int(num);
                    k = i;
                    break;
                }
                i++;
            }
        }
        
            while((len - written) != 0){
                if(freeSpaceInBlock != 0){
                char c;
                if(fseek(sim_disk_fd, (currentBlock * block_size) +(block_size - freeSpaceInBlock), SEEK_SET) != 0){
                    perror("fseek failed");
                    exit(EXIT_FAILURE);
                }
                c = buf[j];
                if(fwrite(&c, 1, 1, sim_disk_fd) == 0){
                    perror("fwrite failed");
                    exit(EXIT_FAILURE);
                }
                j++;
                written++;
                toUse->setFileSize(toUse->getFileSize() + 1);
                freeSpaceInBlock--;
                }
               else if(freeSpaceInBlock == 0 && written != len){
                    if(toUse->getBlocksInUse() < direct_enteris ){
                        toUse->getDirectBlocks()[toUse->getBlocksInUse()] = findEmptyBlock();
                        currentBlock = toUse->getDirectBlocks()[toUse->getBlocksInUse()];
                        toUse->incrementBolcksinUse();
                        freeSpaceInBlock = block_size;
                    }
                    else {
                        char num;
                        if(toUse->getSingleInDirect()== -1){
                            toUse->setSingleInDirect(findEmptyBlockFromTheEnd());
                        }
                        if(fseek(sim_disk_fd, (toUse->getSingleInDirect() * block_size) + k, SEEK_SET) != 0){
                            perror("fseek failed");
                            exit(EXIT_FAILURE);
                        }
                        if(fread(&num, 1, 1, sim_disk_fd) == 0){
                            perror("fread failed");
                            exit(EXIT_FAILURE);
                        }
                        if(num == '\0'){
                           if(fseek(sim_disk_fd, (toUse->getSingleInDirect() * block_size) + k, SEEK_SET) != 0){
                               perror("fseek failed");
                               exit(EXIT_FAILURE);
                           }
                           decToBinary(findEmptyBlock(), num);
                           currentBlock = int(num);
                           if(fwrite(&num, 1, 1, sim_disk_fd) == 0){
                               perror("fwrite failed");
                               exit(EXIT_FAILURE);
                           }
                           freeSpaceInBlock = block_size;
                           toUse->incrementBolcksinUse();
                        }
                        k++;
                    }
                }
            }
            fflush(sim_disk_fd);
            return len;
        

  
    }
    // ------------------------------------------------------------------------
    //this function will try to delete a file (if the file with the given name dose not exit or have been deleted already the function will return -1 )
    // otherwise the function will return the FD of the file with the given name 
    int DelFile( string FileName ) {
        int i = 0;
        int j = 0;
        int k = 0;
        int deleted = 0;
        int fileIndex = -1;
        int currentBlock = -1;
        char num;
        int size = this->OpenFileDescriptors.size();
        map<string, fsInode*>::iterator it;
        fsInode *toUse = nullptr;
        it = this->MainDir.find(FileName);
        if(it == this->MainDir.end()){
            fprintf(stderr, "file dose not exist\n");
            return -1;
        }
        for(i = 0; i < size; i++){
            if(!this->OpenFileDescriptors[i].isDeleted() && this->OpenFileDescriptors[i].getFileName() == FileName){
                toUse = this->OpenFileDescriptors[i].getInode();
                fileIndex = i;
                break;
            }
        }
        if(toUse == nullptr){
            fprintf(stderr, "file has been deleted already\n");
            return -1;
        }
        int fileSize = toUse->getFileSize();
        if(fileSize == 0){
            delete toUse;
            this->MainDir.erase(FileName);
            this->OpenFileDescriptors[fileIndex].setDeletedStat(true);
            this->OpenFileDescriptors[fileIndex].setInUse(false);
            return fileIndex;
        }

        i = 1;
        k = 0;
        j =0;
        currentBlock = toUse->getDirectBlocks()[0];
        while((fileSize - deleted) != 0){
            if(fseek(sim_disk_fd, (block_size * currentBlock) + j, SEEK_SET) != 0){
                perror("fseek failed");
                exit(EXIT_FAILURE);
            }
            if(fwrite("\0", 1, 1, sim_disk_fd) == 0){
                perror("fwrite failed");
                exit(EXIT_FAILURE);
            }
            deleted++;
            j++;
            if((deleted % block_size) == 0 && deleted != 0 && (fileSize - deleted) != 0){
                if(i < direct_enteris){
                    this->BitVector[currentBlock] = 0;
                    currentBlock = toUse->getDirectBlocks()[i];
                    i++;
                    j = 0;
                }
                else{
                    if(fseek(sim_disk_fd, (block_size * toUse->getSingleInDirect()) + k, SEEK_SET) != 0){
                        perror("fseek failed");
                    }
                    if(fread(&num, 1, 1, sim_disk_fd) == 0){
                        perror("fread failed");
                        exit(EXIT_FAILURE);
                    }  
                    this->BitVector[currentBlock] = 0;
                    currentBlock = (int)num;
                    k++;
                    j = 0;
                }
            }
        }
        this->BitVector[currentBlock] = 0;
        if(toUse->getSingleInDirect()!= -1){
        this->BitVector[toUse->getSingleInDirect()] = 0;
        i = 0;
        while(i < block_size){
            if(fseek(sim_disk_fd, (block_size * toUse->getSingleInDirect()) + i, SEEK_SET) != 0){
                perror("fseek failed");
                exit(EXIT_FAILURE);
            }
            if(fwrite("\0", 1, 1, sim_disk_fd) == 0){
                perror("fwrite failed");
                exit(EXIT_FAILURE);
            }
            i++;
        }
        }
        fflush(sim_disk_fd);
        delete toUse;
        this->MainDir.erase(FileName);
        this->OpenFileDescriptors[fileIndex].setDeletedStat(true);
        this->OpenFileDescriptors[fileIndex].setInUse(false);
        return fileIndex;
    }
  
     // ------------------------------------------------------------------------
    //this function will try to read a given number from a virtual_file if the file is empty or we are trying to read more than the disk size the funtion will return -1
    // otherwise the function will return the number of the readen bits
    int ReadFromFile(int fd, char *buf, int len ) {
        int size = this->OpenFileDescriptors.size();
        int read = 0;
        char r;
        char num;
        int i = 0;
        int j = 0;
        int k = 0;
        int currentBlock = 0;
        for(i = 0; i < len; i++){
            buf[i] = '\0';
        } 
        if(len == 0){
            buf[0] ='\0';
        }
        i = 0;
        if(!this->is_formated){
          fprintf(stderr, "disk is not formatted yet\n");
          return -1;
        }
       if(fd < 0 || fd >= size){
          fprintf(stderr, "file dose not exist\n");
          return -1;
        }
       if(len > DISK_SIZE || len <= 0){
          fprintf(stderr, "can't read all necesary data(because its bigger than the disk size or equal to zero)\n");
          return -1;
        }

      string fileName = this->OpenFileDescriptors[fd].getFileName();
      map<string, fsInode*>::iterator it;
      it = this->MainDir.find(fileName);
      if(it == this->MainDir.end()){
          fprintf(stderr, "file deleted\n");
          return -1;
      }
      fsInode *toUse = this->MainDir[fileName];
      if(toUse->getFileSize() <= 0){
          fprintf(stderr, "can not read a file of size 0\n");
          return -1;
      }
      if(toUse->getFileSize() < len){
          len = toUse->getFileSize();
      }
      i = 0;
      currentBlock = toUse->getDirectBlocks()[i];
      i++;
      while((len - read) != 0){
          if(fseek(sim_disk_fd, (block_size * currentBlock) + k, SEEK_SET) != 0){
              perror("fseek failed");
              exit(EXIT_FAILURE);
          }
          if(fread(&r, 1, 1, sim_disk_fd) == 0){
              perror("fread failed");
              exit(EXIT_FAILURE);
          }
          buf[read] = r;
          read++;
          k++;
          if((read % block_size) == 0 && read != 0  && (len - read) != 0){
              if( i < direct_enteris){
                  currentBlock = toUse->getDirectBlocks()[i];
                  i++;
                  k = 0;
              }
              else{
                  if(fseek(sim_disk_fd, (block_size * toUse->getSingleInDirect()) + j,SEEK_SET) != 0){
                      perror("fseek failed");
                      exit(EXIT_FAILURE);
                  }
                  if(fread(&num,1 , 1, sim_disk_fd) == 0){
                      perror("fread failed");
                      exit(EXIT_FAILURE);
                  }
                  currentBlock = (int)num;
                  j++;
                  k = 0;
              }
          }
      }
      buf[read] = '\0';
      return len;
	  
	  
    }
  
  ~fsDisk(){
      fsInode* toUse;
      if (this->MainDir.empty()){
          fclose(sim_disk_fd);
          return;
      }
      map<string, fsInode*>::iterator it = this->MainDir.begin();
      while(it != this->MainDir.end()){
          toUse = it->second;
          delete toUse;
          it++;
      }
      this->MainDir.clear();
      this->OpenFileDescriptors.clear();
      delete [] BitVector;
      fclose(sim_disk_fd);
  }


    private:
    //this function will find an empty block from the end of the disk
    int findEmptyBlock(){
        int i;
        for(i = 0; i < this->BitVectorSize; i++){
            if(this->BitVector[i] == 0){
                this->BitVector[i] = 1;
                return i;
            }
        }
        return -1;
    }

    //this function will find how many empty blocks left in the disk
    int howManyEmptyBlocks(){
        int i;
        int toReturn = 0;
        for(i = 0; i < this->BitVectorSize; i++){
            if(this->BitVector[i] == 0){
                toReturn ++;
            }
        }
        return toReturn;
    }
    //this fucntion will find how many an empty block starting searching from the bigenning of the file
    int findEmptyBlockFromTheEnd(){
         int i;
        for(i = BitVectorSize - 1; i >= 0; i--){
            if(this->BitVector[i] == 0){
                this->BitVector[i] = 1;
                return i;
            }
        }
        return -1;
    }


};
    
int main() {
    int blockSize; 
	int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read; 
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
				delete fs;
				exit(0);
                break;

            case 1:  // list-file
                fs->listAll(); 
                break;
          
            case 2:    // format
                cin >> blockSize;
				cin >> direct_entries;
                fs->fsFormat(blockSize, direct_entries);
                break;
          
            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            
            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
             
            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd); 
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
           
            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
                break;
          
            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;
                break;
           
            case 8:   // delete file 
                 cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }
    

}

