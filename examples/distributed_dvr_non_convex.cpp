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

    // Command-line argument parsing
    if (argc != 5) {
        std::cerr << "Usage: mpirun -np <num_processes> ./program"
                     " <data_directory> <dataset_name> <width> <height>" << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    std::string dataDirectory = argv[1];
    std::string datasetName = argv[2];
    const auto width = std::atoi(argv[3]);
    const auto height = std::atoi(argv[4]);

    char const* className = getenv("LIV_RENDERER_CLASS");

    if (className == nullptr) {
        className = "NonConvexVolumesInterface";
    }

    auto livEngine = liv::LiVEngine::initialize(width, height, className, datasetName);

    int rank, numProcs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    const auto numLayers = std::atoi(getEnvVar("LIV_NUM_LAYERS"));
    int numBlocks = numLayers * numProcs;

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

    int sizeX, sizeY, sizeZ, datatypeValue;
    infoFile >> sizeX >> sizeY >> sizeZ >> datatypeValue;

    if (infoFile.fail()) {
        std::cerr << "Error: Failed to read block info from file: " << infoFilePath << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (datatypeValue != 8 && datatypeValue != 16) {
        std::cerr << "Error: Invalid datatype value in .info file: " << datatypeValue << ". Please ensure that the datatype value is either 8 or 16." << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    infoFile.close();

    std::vector<int> datasetDimensions = {sizeX, sizeY, sizeZ};

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

    // Assign and load blocks.
    using namespace std::string_literals;

    struct Block {
        std::vector<char> data;
        BlockInfo info;
        int index;
    };
    std::vector<Block> blocks {static_cast<std::size_t>(numLayers)};
    auto path = std::filesystem::path(blocksDirectory) / "";

    for (int layer = 0; layer < numLayers; ++layer) {
        auto& block = blocks[layer];
        block.index = layer*numProcs + (rank + layer) % numProcs;

        path.replace_filename("block_"s + std::to_string(block.index) + ".info");
        if (!readBlockInfo(path, blocks[layer].info)) {
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        path.replace_extension(".raw");
        if (!readBlockData(path, blocks[layer].data)) {
            MPI_Finalize();
            return EXIT_FAILURE;
        }
    }

    // Count block adjacencies.
    int num_adjacent = 0;
    int num_non_adjavent = 0;

    for (int i = 0; i < blocks.size(); ++i) {
        for (int j = i + 1; j < blocks.size(); ++j) {
            std::cout << "Process " << rank << ": Blocks " << blocks[i].index << " and " << blocks[j].index;

            if (areBlocksAdjacent(blocks[i].info, blocks[j].info)) {
                std::cout << " are adjacent." << std::endl;
                ++num_adjacent;
            } else {
                std::cout << " are non-adjacent." << std::endl;
                ++num_non_adjavent;
            }
        }
    }

    std::cout << "Process " << rank << " has " << num_adjacent << " adjacent, "
              << num_non_adjavent << " non-adjacent pairs of blocks.\n";

    std::thread renderThread([&livEngine]() { livEngine.doRender(); });

    livEngine.setVolumeDimensions(datasetDimensions);

    float pixelToWorld = livEngine.getVolumeScaling();

    // Print assigned blocks, transform to world coordinates.
    std::cout << "Process " << rank << " loaded blocks:" << std::endl;

    for (auto& block : blocks) {
        auto& info = block.info;

        info.posX *=  pixelToWorld;
        info.posY *= -pixelToWorld;
        info.posZ *=  pixelToWorld;

        std::cout << "  Block " << block.index << ":" << std::endl;
        std::cout << "    Size: " << info.sizeX << " x " << info.sizeY << " x " << info.sizeZ << std::endl;
        std::cout << "    Position: (" << info.posX << ", " << info.posY << ", " << info.posZ << ")" << std::endl;
    }

    // Pass volume data to renderer.
    if(datatypeValue == 8) {
        for (auto& block : blocks) {
            liv::createVolume<char>(&block.info.posX, &block.info.sizeX, &livEngine)
                .update(block.data.data(), block.data.size());
        }
    } else {
        for (auto& block : blocks) {
            liv::createVolume<unsigned short>(&block.info.posX, &block.info.sizeX, &livEngine)
                .update(reinterpret_cast<unsigned short*>(block.data.data()), block.data.size());
        }
    }

    // Run renderer.
    livEngine.setSceneConfigured();
    renderThread.join();

    // Finalize MPI
    MPI_Finalize();
    return EXIT_SUCCESS;
}
