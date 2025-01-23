#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <liv.h>
#include <thread>
#include <filesystem>

typedef char datatype;

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

    std::string infoFilePath;
    for (const auto& entry : std::filesystem::directory_iterator(dataDirectory)) {
        if (entry.path().extension() == ".info") {
            infoFilePath = entry.path().string();
            break;
        }
    }
    if (infoFilePath.empty()) {
        std::cerr << "Error: No .info file found in directory: " << dataDirectory << ". Please ensure that there is a .info file containing the dimensions of the dataset as comma-seperated integers." << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    std::ifstream infoFile(infoFilePath);
    if (!infoFile.is_open()) {
        std::cerr << "Error: Could not open info file: " << infoFilePath << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    std::string line;
    std::getline(infoFile, line);
    std::stringstream ss(line);
    std::vector<int> datasetDimensions;
    std::string value;
    while (std::getline(ss, value, ',')) {
        datasetDimensions.push_back(std::stoi(value));
    }
    infoFile.close();

    // Construct the blocks directory path
    std::string blocksDirectory = dataDirectory + "/blocks" + std::to_string(numProcs);

    // Check if blocks directory exists
    if (!directoryExists(blocksDirectory)) {
        if (rank == 0) {
            std::cerr << "Error: Blocks directory does not exist: " << blocksDirectory << std::endl;
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Ensure that the rank does not exceed the number of blocks
    if (rank >= numProcs) {
        std::cerr << "Error: MPI rank " << rank << " exceeds the number of available blocks (" << numProcs << ")." << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Construct the file paths for the block data and info files
    std::string blockFileName = blocksDirectory + "/block_" + std::to_string(rank) + ".raw";
    std::string infoFileName = blocksDirectory + "/block_" + std::to_string(rank) + ".info";


    // Read the block information
    BlockInfo blockInfo;
    if (!readBlockInfo(infoFileName, blockInfo)) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Read the block data
    std::vector<char> blockData;
    if (!readBlockData(blockFileName, blockData)) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    std::thread renderThread([&livEngine]() { livEngine.doRender(); });

    livEngine.setVolumeDimensions(datasetDimensions);
    livEngine.addProcessorData(rank, {blockInfo.posX, blockInfo.posY, blockInfo.posZ}, {static_cast<float>(blockInfo.sizeX), static_cast<float>(blockInfo.sizeY), static_cast<float>(blockInfo.sizeZ)});

    float pixelToWorld = livEngine.getVolumeScaling();

    blockInfo.posX *= pixelToWorld;
    blockInfo.posY *= (-1 * pixelToWorld);
    blockInfo.posZ *= pixelToWorld;

    // Now you have:
    // - blockData: vector<char> containing the raw data of the block
    // - blockInfo: BlockInfo struct containing size, bit resolution, and position

    // For demonstration purposes, let's print out the block info from each rank
    std::cout << "Process " << rank << " loaded block data:" << std::endl;
    std::cout << "  Size: " << blockInfo.sizeX << " x " << blockInfo.sizeY << " x " << blockInfo.sizeZ << std::endl;
    std::cout << "  Bit Resolution: " << blockInfo.bitResolution << std::endl;
    std::cout << "  Position: (" << blockInfo.posX << ", " << blockInfo.posY << ", " << blockInfo.posZ << ")" << std::endl;
    std::cout << "  Data Size: " << blockData.size() << " bytes" << std::endl;

    float position[3] = {blockInfo.posX, blockInfo.posY, blockInfo.posZ};
    int dimensions[3] = {blockInfo.sizeX, blockInfo.sizeY, blockInfo.sizeZ};

    auto volume = liv::createVolume<datatype>(position, dimensions, &livEngine);

    volume.update(reinterpret_cast<datatype*>(blockData.data()), blockData.size());

    livEngine.setSceneConfigured();

    // Allow some time for the rendering to take place
    std::this_thread::sleep_for(std::chrono::seconds(100));

    renderThread.join();

    // Finalize MPI
    MPI_Finalize();
    return EXIT_SUCCESS;
}