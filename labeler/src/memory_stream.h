#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

#include "mip/stream.h"

/// In-memory Stream implementation — decrypted content never touches disk.
class MemoryStream : public mip::Stream {
public:
    MemoryStream() : mPos(0) {}

    int64_t Read(uint8_t* buffer, int64_t bufferLength) override {
        int64_t available = static_cast<int64_t>(mData.size()) - mPos;
        int64_t toRead = std::min(bufferLength, available);
        if (toRead <= 0) return 0;
        std::memcpy(buffer, mData.data() + mPos, static_cast<size_t>(toRead));
        mPos += toRead;
        return toRead;
    }

    int64_t Write(const uint8_t* buffer, int64_t bufferLength) override {
        int64_t endPos = mPos + bufferLength;
        if (endPos > static_cast<int64_t>(mData.size())) {
            mData.resize(static_cast<size_t>(endPos));
        }
        std::memcpy(mData.data() + mPos, buffer, static_cast<size_t>(bufferLength));
        mPos += bufferLength;
        return bufferLength;
    }

    bool Flush() override { return true; }

    void Seek(int64_t position) override { mPos = position; }

    bool CanRead()  const override { return true; }
    bool CanWrite() const override { return true; }

    int64_t Position() override { return mPos; }

    int64_t Size() override { return static_cast<int64_t>(mData.size()); }
    void    Size(int64_t value) override { mData.resize(static_cast<size_t>(value)); }

    const std::vector<uint8_t>& Data() const { return mData; }

private:
    std::vector<uint8_t> mData;
    int64_t mPos;
};
