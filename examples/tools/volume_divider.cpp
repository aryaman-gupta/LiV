#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <sys/stat.h> // For mkdir
#include <sys/types.h>

#if defined(_WIN32)
#include <direct.h>   // For _mkdir
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0755)
#endif

struct VolumeInfo {
    int x_dim;
    int y_dim;
    int z_dim;
    int bit_resolution;
};

bool parseInfoFile(const std::string& infoFilePath, VolumeInfo& volInfo) {
    std::ifstream infoFile(infoFilePath);
    if (!infoFile.is_open()) {
        std::cerr << "Error: Could not open .info file: " << infoFilePath << std::endl;
        return false;
    }
    infoFile >> volInfo.x_dim >> volInfo.y_dim >> volInfo.z_dim;
    infoFile >> volInfo.bit_resolution;
    infoFile.close();
    return true;
}

void computeSubdivision(int numDivisions, int& nx, int& ny, int& nz) {
    int n = std::round(std::pow(numDivisions, 1.0 / 3.0));
    nx = ny = nz = n;

    // Adjust nx, ny, nz to ensure that nx * ny * nz >= numDivisions
    while (nx * ny * nz < numDivisions) {
        if (nx <= ny && nx <= nz)
            nx++;
        else if (ny <= nx && ny <= nz)
            ny++;
        else
            nz++;
    }
}

bool readVolumeData(const std::string& volumeFilePath, std::vector<char>& volumeData, const VolumeInfo& volInfo) {
    std::ifstream volumeFile(volumeFilePath, std::ios::binary);
    if (!volumeFile.is_open()) {
        std::cerr << "Error: Could not open volume file: " << volumeFilePath << std::endl;
        return false;
    }

    size_t dataSize = static_cast<size_t>(volInfo.x_dim) * volInfo.y_dim * volInfo.z_dim * (volInfo.bit_resolution / 8);
    volumeData.resize(dataSize);

    volumeFile.read(reinterpret_cast<char*>(volumeData.data()), dataSize);
    if (!volumeFile) {
        std::cerr << "Error: Could not read volume data." << std::endl;
        return false;
    }
    volumeFile.close();
    return true;
}

bool writeBlockData(const std::string& blockDir, int blockIndex, const std::vector<char>& blockData,
                    int bx, int by, int bz, const VolumeInfo& volInfo, int startX, int startY, int startZ) {
    std::string blockFileName = blockDir + "/block_" + std::to_string(blockIndex) + ".raw";
    std::ofstream blockFile(blockFileName, std::ios::binary);
    if (!blockFile.is_open()) {
        std::cerr << "Error: Could not open block file for writing: " << blockFileName << std::endl;
        return false;
    }

    blockFile.write(reinterpret_cast<const char*>(blockData.data()), blockData.size());
    blockFile.close();

    // Write the .info file for this block
    std::string infoFileName = blockDir + "/block_" + std::to_string(blockIndex) + ".info";
    std::ofstream infoFile(infoFileName);
    if (!infoFile.is_open()) {
        std::cerr << "Error: Could not open block info file for writing: " << infoFileName << std::endl;
        return false;
    }
    // Write size
    infoFile << bx << " " << by << " " << bz << std::endl;
    // Write bit resolution
    infoFile << volInfo.bit_resolution << std::endl;
    // Write location (Front Bottom Left coordinates)
    infoFile << startX << " " << startY << " " << startZ << std::endl;
    infoFile.close();

    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: volume_divider <volume_file_path> <base_output_directory> <number_of_divisions>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string volumeFilePath = argv[1];
    std::string baseOutputDir = argv[2];
    int numDivisions = std::atoi(argv[3]);

    // Ensure number of divisions is positive
    if (numDivisions <= 0) {
        std::cerr << "Error: Number of divisions must be a positive integer." << std::endl;
        return EXIT_FAILURE;
    }

    // Read the .info file
    std::string infoFilePath = volumeFilePath.substr(0, volumeFilePath.find_last_of('.')) + ".info";
    VolumeInfo volInfo;
    if (!parseInfoFile(infoFilePath, volInfo)) {
        return EXIT_FAILURE;
    }

    // Read the volume data
    std::vector<char> volumeData;
    if (!readVolumeData(volumeFilePath, volumeData, volInfo)) {
        return EXIT_FAILURE;
    }

    // Compute the subdivision
    int nx, ny, nz;
    computeSubdivision(numDivisions, nx, ny, nz);

    // Compute block sizes
    int baseBlockSizeX = volInfo.x_dim / nx;
    int baseBlockSizeY = volInfo.y_dim / ny;
    int baseBlockSizeZ = volInfo.z_dim / nz;

    int remainderX = volInfo.x_dim % nx;
    int remainderY = volInfo.y_dim % ny;
    int remainderZ = volInfo.z_dim % nz;

    // Create the dataset directory within the base output directory
    std::string volumeFileName = volumeFilePath.substr(volumeFilePath.find_last_of("/\\") + 1);
    std::string volumeName = volumeFileName.substr(0, volumeFileName.find_last_of('.'));
    std::string datasetDir = baseOutputDir + "/" + volumeName;

    // Create directories if they don't exist
    MKDIR(baseOutputDir.c_str());
    MKDIR(datasetDir.c_str());

    // Create the blocksN directory
    std::string blockDirName = datasetDir + "/blocks" + std::to_string(numDivisions);
    MKDIR(blockDirName.c_str());

    // Divide the volume into blocks
    int blockIndex = 0;
    for (int bz = 0; bz < nz; ++bz) {
        int startZ = bz * baseBlockSizeZ + std::min(bz, remainderZ);
        int endZ = startZ + baseBlockSizeZ + (bz < remainderZ ? 1 : 0);

        for (int by = 0; by < ny; ++by) {
            int startY = by * baseBlockSizeY + std::min(by, remainderY);
            int endY = startY + baseBlockSizeY + (by < remainderY ? 1 : 0);

            for (int bx = 0; bx < nx; ++bx) {
                int startX = bx * baseBlockSizeX + std::min(bx, remainderX);
                int endX = startX + baseBlockSizeX + (bx < remainderX ? 1 : 0);

                int blockSizeX = endX - startX;
                int blockSizeY = endY - startY;
                int blockSizeZ = endZ - startZ;

                // Extract the block data
                std::vector<char> blockData;
                for (int z = startZ; z < endZ; ++z) {
                    for (int y = startY; y < endY; ++y) {
                        size_t offset = ((size_t)z * volInfo.y_dim * volInfo.x_dim) + (y * volInfo.x_dim) + startX;
                        size_t length = blockSizeX;
                        size_t dataOffset = offset * (volInfo.bit_resolution / 8);
                        blockData.insert(blockData.end(),
                                         volumeData.begin() + dataOffset,
                                         volumeData.begin() + dataOffset + length * (volInfo.bit_resolution / 8));
                    }
                }

                // Write the block data to a file
                if (!writeBlockData(blockDirName, blockIndex, blockData, blockSizeX, blockSizeY, blockSizeZ,
                                    volInfo, startX, startY, startZ)) {
                    return EXIT_FAILURE;
                }

                blockIndex++;
                if (blockIndex >= numDivisions) {
                    break;
                }
            }
            if (blockIndex >= numDivisions) {
                break;
            }
        }
        if (blockIndex >= numDivisions) {
            break;
        }
    }

    std::cout << "Volume divided into " << blockIndex << " blocks." << std::endl;
    return EXIT_SUCCESS;
}