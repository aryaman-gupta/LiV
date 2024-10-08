#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <liv.h>
#include <thread>

typedef unsigned short datatype;

struct BlockInfo {
    int sizeX;
    int sizeY;
    int sizeZ;
    int bitResolution;
    float posX;
    float posY;
    float posZ;
};

// Function to read the block data from the .raw file
bool readBlockData(const std::string& blockFilePath, std::vector<char>& blockData) {
    std::ifstream blockFile(blockFilePath, std::ios::binary);
    if (!blockFile.is_open()) {
        std::cerr << "Error: Could not open block file: " << blockFilePath << std::endl;
        return false;
    }

    // Get the size of the file
    blockFile.seekg(0, std::ios::end);
    std::streamsize dataSize = blockFile.tellg();
    blockFile.seekg(0, std::ios::beg);

    blockData.resize(dataSize);
    if (!blockFile.read(blockData.data(), dataSize)) {
        std::cerr << "Error: Could not read block data from file: " << blockFilePath << std::endl;
        return false;
    }
    blockFile.close();
    return true;
}

// Function to read the block information from the .info file
bool readBlockInfo(const std::string& infoFilePath, BlockInfo& blockInfo) {
    std::ifstream infoFile(infoFilePath);
    if (!infoFile.is_open()) {
        std::cerr << "Error: Could not open block info file: " << infoFilePath << std::endl;
        return false;
    }

    infoFile >> blockInfo.sizeX >> blockInfo.sizeY >> blockInfo.sizeZ;
    infoFile >> blockInfo.bitResolution;
    infoFile >> blockInfo.posX >> blockInfo.posY >> blockInfo.posZ;

    if (infoFile.fail()) {
        std::cerr << "Error: Failed to read block info from file: " << infoFilePath << std::endl;
        return false;
    }

    infoFile.close();
    return true;
}

// Function to check if a directory exists
bool directoryExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    else
        return (info.st_mode & S_IFDIR) != 0;
}

bool areBlocksAdjacent(const BlockInfo& block1, const BlockInfo& block2) {
    // Check adjacency along X axis
    bool x_adjacent = (block1.posX + block1.sizeX == block2.posX || block2.posX + block2.sizeX == block1.posX) &&
                      (block1.posY < block2.posY + block2.sizeY && block1.posY + block1.sizeY > block2.posY) &&
                      (block1.posZ < block2.posZ + block2.sizeZ && block1.posZ + block1.sizeZ > block2.posZ);

    // Check adjacency along Y axis
    bool y_adjacent = (block1.posY + block1.sizeY == block2.posY || block2.posY + block2.sizeY == block1.posY) &&
                      (block1.posX < block2.posX + block2.sizeX && block1.posX + block1.sizeX > block2.posX) &&
                      (block1.posZ < block2.posZ + block2.sizeZ && block1.posZ + block1.sizeZ > block2.posZ);

    // Check adjacency along Z axis
    bool z_adjacent = (block1.posZ + block1.sizeZ == block2.posZ || block2.posZ + block2.sizeZ == block1.posZ) &&
                      (block1.posX < block2.posX + block2.sizeX && block1.posX + block1.sizeX > block2.posX) &&
                      (block1.posY < block2.posY + block2.sizeY && block1.posY + block1.sizeY > block2.posY);

    // The blocks are adjacent if they are adjacent along any one axis
    return x_adjacent || y_adjacent || z_adjacent;
}


int main(int argc, char* argv[]) {

    auto livEngine = liv::LiVEngine::initialize(1280, 720);

    int rank, numProcs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    // Command-line argument parsing
    if (argc != 2) {
        if (rank == 0) {
            std::cerr << "Usage: mpirun -np <num_processes> ./program <data_directory>" << std::endl;
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    std::string dataDirectory = argv[1];

    int numBlocks = 2 * numProcs;

    // Construct the blocks directory path
    std::string blocksDirectory = dataDirectory + "/blocks" + std::to_string(numBlocks);

    // Check if blocks directory exists
    if (!directoryExists(blocksDirectory)) {
        if (rank == 0) {
            std::cerr << "Error: Blocks directory does not exist: " << blocksDirectory << std::endl;
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Each process will load two blocks based on the assignment strategy
    int blockIndex1 = rank;            // From Group A (indices 0 to n-1)
    int blockIndex2 = (2*numProcs) - 1 - rank; // From Group B (indices n to 2n-1)

    // Construct the file paths for the block data and info files
    std::string blockFileName1 = blocksDirectory + "/block_" + std::to_string(blockIndex1) + ".raw";
    std::string infoFileName1 = blocksDirectory + "/block_" + std::to_string(blockIndex1) + ".info";

    std::string blockFileName2 = blocksDirectory + "/block_" + std::to_string(blockIndex2) + ".raw";
    std::string infoFileName2 = blocksDirectory + "/block_" + std::to_string(blockIndex2) + ".info";

    // Read the block information for both blocks
    BlockInfo blockInfo1, blockInfo2;
    if (!readBlockInfo(infoFileName1, blockInfo1)) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    if (!readBlockInfo(infoFileName2, blockInfo2)) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Read the block data for both blocks
    std::vector<char> blockData1, blockData2;
    if (!readBlockData(blockFileName1, blockData1)) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    if (!readBlockData(blockFileName2, blockData2)) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (areBlocksAdjacent(blockInfo1, blockInfo2)) {
        std::cout << "Process " << rank << ": Assigned blocks are adjacent." << std::endl;
    } else {
        std::cout << "Process " << rank << ": Assigned blocks are non-adjacent." << std::endl;
    }

    float pixelToWorld = 3.84f/1024.0f;

    blockInfo1.posX *= pixelToWorld;
    blockInfo1.posY *= (-1 * pixelToWorld);
    blockInfo1.posZ *= pixelToWorld;

    blockInfo2.posX *= pixelToWorld;
    blockInfo2.posY *= (-1 * pixelToWorld);
    blockInfo2.posZ *= pixelToWorld;

    // Now you have:
    // - blockData1 and blockData2: vectors containing the raw data of the blocks
    // - blockInfo1 and blockInfo2: BlockInfo structs containing size, bit resolution, and position

    // For demonstration purposes, let's print out the block info from each rank
    std::cout << "Process " << rank << " loaded blocks:" << std::endl;
    std::cout << "  Block " << blockIndex1 << ":" << std::endl;
    std::cout << "    Size: " << blockInfo1.sizeX << " x " << blockInfo1.sizeY << " x " << blockInfo1.sizeZ << std::endl;
    std::cout << "    Position: (" << blockInfo1.posX << ", " << blockInfo1.posY << ", " << blockInfo1.posZ << ")" << std::endl;

    std::cout << "  Block " << blockIndex2 << ":" << std::endl;
    std::cout << "    Size: " << blockInfo2.sizeX << " x " << blockInfo2.sizeY << " x " << blockInfo2.sizeZ << std::endl;
    std::cout << "    Position: (" << blockInfo2.posX << ", " << blockInfo2.posY << ", " << blockInfo2.posZ << ")" << std::endl;

    std::thread renderThread([&livEngine]() { livEngine.doRender(); });

    float position1[3] = {blockInfo1.posX, blockInfo1.posY, blockInfo1.posZ};
    int dimensions1[3] = {blockInfo1.sizeX, blockInfo1.sizeY, blockInfo1.sizeZ};

    float position2[3] = {blockInfo2.posX, blockInfo2.posY, blockInfo2.posZ};
    int dimensions2[3] = {blockInfo2.sizeX, blockInfo2.sizeY, blockInfo2.sizeZ};

    auto volume1 = liv::createVolume<datatype>(position1, dimensions1, &livEngine);
    auto volume2 = liv::createVolume<datatype>(position2, dimensions2, &livEngine);

    volume1.update(reinterpret_cast<datatype*>(blockData1.data()), blockData1.size());
    volume2.update(reinterpret_cast<datatype*>(blockData2.data()), blockData2.size());

    livEngine.setSceneConfigured();

    renderThread.join();

    // Finalize MPI
    MPI_Finalize();
    return EXIT_SUCCESS;
}
