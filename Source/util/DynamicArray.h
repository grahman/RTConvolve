//
//  DynamicArray.h
//  RTConvolve
//
//  Created by Graham Barab on 4/10/26.
//  Copyright © 2026 Graham Barab. All rights reserved.
//

#ifndef DynamicArray_h
#define DynamicArray_h

template <class T>
class DynamicArray
{
    std::unique_ptr<T[]> mData;
    size_t mCount {0};
    
public:
    DynamicArray() = default;
    
    DynamicArray(size_t count)
    : mData{new T[count]}
    , mCount(count)
    {}
    
    DynamicArray(DynamicArray&& rhs)
    : mData{std::move(rhs.mData)}
    , mCount{rhs.mCount}
    {
        rhs.mCount = 0;
    }
    
    DynamicArray& operator = (DynamicArray& rhs)
    {
        mData = std::move(rhs.mData);
        mCount = rhs.mCount;
        rhs.mCount = 0;
        return *this;
    }
    
    void reset(size_t count)
    {
        mData.reset();
        mCount = count;
        if (count)
        {
            mData.reset(new T[count]);
        }
    }
    
    T* data() const { return mData.get(); }
    size_t size() const { return mCount; }
    
    T* begin() const { return data(); }
    T* end() const { return data() + size(); }
};

#endif /* DynamicArray_h */
