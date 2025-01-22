//
// Created by aryaman on 24.04.24.
//

#ifndef MANAGERENDERING_H
#define MANAGERENDERING_H

#include "JVMData.h"
#include <vector>
namespace liv {

    class RenderingManager {
        bool rendererConfigured = false;
        JVMData* jvmData;

    public:
        explicit RenderingManager(JVMData* jvmData) : jvmData(jvmData) {}

        [[nodiscard]] bool isRendererConfigured() const;

        void doRender();

        void setVolumeDimensions(const std::vector<int>& dimensions);

        float getVolumeScaling();

        void addProcessorData(int processorID, const std::vector<float>& origin, const std::vector<float>& dimensions);

        void setSceneConfigured();

        void waitRendererConfigured();

        void stopRendering();

        void setupICET();
    };
}
#endif //MANAGERENDERING_H
