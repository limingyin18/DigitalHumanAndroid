//
// Created by ylm on 4/4/2021.
//

#ifndef HELLOWORLDC_IMAGEREADER_H
#define HELLOWORLDC_IMAGEREADER_H

#include <vector>
#include <memory>
#include <iostream>
#include <cassert>

#include <media/NdkImage.h>
#include <media/NdkImageReader.h>

/**
 * camera image target window
 * use AHardwareBuffer to communicate with Vulkan
 */
class ImageReader
{
public:
    unsigned cur_index_;
    ANativeWindow* window_ = nullptr;
    std::vector<AHardwareBuffer*> buffers_;

    std::unique_ptr<AImageReader, decltype(&AImageReader_delete)> reader_;
    std::vector<std::unique_ptr<AImage, decltype(&AImage_delete)>> images_;

public:
    ImageReader(unsigned width, unsigned height, unsigned format, unsigned long usage,
            unsigned max_images);
    ~ImageReader();

    AHardwareBuffer* GetLatestBuffer();
    ANativeWindow* GetWindow() const noexcept;
};


#endif //HELLOWORLDC_IMAGEREADER_H
