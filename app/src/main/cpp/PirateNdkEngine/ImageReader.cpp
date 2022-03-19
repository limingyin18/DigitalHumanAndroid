//
// Created by ylm on 4/4/2021.
//

#include "ImageReader.h"

using namespace std;

ImageReader::ImageReader(unsigned width, unsigned height, unsigned format, unsigned long usage,
                         unsigned max_images) : cur_index_{max_images - 1},
                                                reader_{nullptr, AImageReader_delete}
{
    assert(max_images >= 2);
    for (unsigned i = 0; i < max_images; ++i)
    {
        images_.emplace_back(nullptr, AImage_delete);
        buffers_.emplace_back(nullptr);
    }

    // create android image reader
    {
        auto pt = reader_.release();
        //auto res = AImageReader_newWithUsage(width, height, format, usage, images_.size() + 2, &pt);
        auto res = AImageReader_new(width, height, format, max_images, &pt);
        if (res != AMEDIA_OK)
        {
            throw runtime_error("can't create android image reader " + to_string(res));
        }
        reader_.reset(pt);
    }

    // get window
    {
        auto res = AImageReader_getWindow(reader_.get(), &window_);

        if (res != AMEDIA_OK || window_ == nullptr)
            throw runtime_error("failed to get window");
    }

    clog << "image reader created" << endl;
}

ImageReader::~ImageReader()
{
    clog << "image reader clear" << endl;
}

ANativeWindow *ImageReader::GetWindow() const noexcept
{
    return window_;
}

AHardwareBuffer *ImageReader::GetLatestBuffer()
{
    AImage *image = nullptr;
    AHardwareBuffer *buffer=nullptr;

    {
        auto res = AImageReader_acquireLatestImage(reader_.get(), &image);
        // acquire image
        if (res != AMEDIA_OK || image == nullptr)
        {
            clog << to_string(res) << endl;
            return nullptr;
        }
    }

    // acquire hardware buffer
    {
        auto res = AImage_getHardwareBuffer(image, &buffer);
        if (res != AMEDIA_OK || buffer == nullptr)
        {
            clog << to_string(res) << endl;
            return nullptr;
        }
    }

    if(++cur_index_ == images_.size())
    {
        cur_index_ = 0;
    }
    images_[cur_index_].reset(image);
    buffers_[cur_index_] = buffer;

    return buffers_[cur_index_];
}
