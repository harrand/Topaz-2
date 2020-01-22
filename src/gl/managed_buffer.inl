#include "gl/object.hpp"
#include "core/debug/assert.hpp"
#include <cstring>

namespace tz::gl
{
    template<tz::gl::BufferType Type>
    ManagedBuffer<Type>::ManagedBuffer(tz::gl::Object& holder): IManagedBuffer(), Buffer<Type>(), holder(holder), regions(), mapped_block(std::nullopt){}

    template<tz::gl::BufferType Type>
    ManagedBufferRegion ManagedBuffer<Type>::region(std::size_t offset_bytes, std::size_t size_bytes, std::string name)
    {
        this->verify_mapped();
        char* mapped_begin = reinterpret_cast<char*>(this->mapped_block.value().begin);
        char* offsetted_begin = mapped_begin + static_cast<std::ptrdiff_t>(offset_bytes);
        auto emplacement_pair = regions.emplace(ManagedBufferRegion{mapped_begin, name.c_str(), {offsetted_begin, size_bytes}}, name);
        return (emplacement_pair.first)->first;
    }

    template<tz::gl::BufferType Type>
    void ManagedBuffer<Type>::erase(const std::string& region_name)
    {
        for(auto& [cur_region, cur_region_name] : this->regions)
        {
            if(region_name == cur_region_name)
            {
                this->regions.erase(cur_region);
                return;
            }
        }
    }

    template<tz::gl::BufferType Type>
    bool ManagedBuffer<Type>::defragment()
    {
        bool movement = false;
        this->verify_mapped();
        // early out if we have full usage already
        if(this->regions_full())
            return false;
        // named after disk defragmentation
        std::size_t byte_count = 0;
        for(auto const& [region, region_name] : this->regions)
        {
            topaz_assert(byte_count < this->mapped_block.value().size(), "tz::gl::ManagedBuffer<Type>::defragment(): Internal error: Byte count exceeded mapping size!");
            std::size_t const region_size = region.block.size();
            movement |= this->relocate_region(region_name, byte_count);
            byte_count += region_size;
        }
        return movement;
    }

    template<tz::gl::BufferType Type>
    std::size_t ManagedBuffer<Type>::regions_usage() const
    {
        std::size_t total_size = 0;
        for(auto const& [region, region_name] : this->regions)
        {  
            total_size += region.block.size();
        }
        return total_size;
    }

    template<tz::gl::BufferType Type>
    bool ManagedBuffer<Type>::regions_full() const
    {
        return this->size() == this->regions_usage();
    }

    template<tz::gl::BufferType Type>
    tz::mem::Block ManagedBuffer<Type>::map(MappingPurpose purpose)
    {
        this->mapped_block = {Buffer<Type>::map(purpose)};
        return this->mapped_block.value();
    }

    template<tz::gl::BufferType Type>
    void ManagedBuffer<Type>::unmap()
    {
        Buffer<Type>::unmap();
        this->regions.clear();
    }

    template<tz::gl::BufferType Type>
    const ManagedBufferRegion& ManagedBuffer<Type>::operator[](const std::string& name) const
    {
        this->verify_mapped();
        auto find_result = this->find_region_iter(name);
        topaz_assert(find_result != this->regions.end(), "tz::gl::ManagedBuffer<Type>::operator[", name, "]: No such region exists with the name \"", name, "\"");
        return find_result->first;
    }

    template<tz::gl::BufferType Type>
    void ManagedBuffer<Type>::verify_mapped() const
    {
        topaz_assert(this->mapped_block.has_value() && this->is_mapped(), "tz::gl::ManagedBuffer<Type>::verify_mapped(): Verification failed due to buffer not being terminal.");
        topaz_assert(this->is_terminal(), "tz::gl::ManagedBuffer<Type>::verify_mapped(): Verification failed due to buffer not being terminal.");
    }

    template<tz::gl::BufferType Type>
    bool ManagedBuffer<Type>::relocate_region(const std::string& region_name, std::size_t byte_index)
    {
        this->verify_mapped();
        void* mapping_begin = this->mapped_block.value().begin;
        MapType::iterator find_result = this->find_region_iter(region_name);
        topaz_assert(find_result != this->regions.end(), "uh oh");
        // for some god-forsaken reason the key appears to be hard-coded as const.
        // so i'll just erase the entry and re-add the new key.
        // inefficient as fuck
        this->regions.erase(find_result);
        auto region = find_result->first;
        auto region_before = region.block;
        const std::size_t region_size_bytes = region_before.size();
        std::size_t byte_index_before = tz::mem::byte_distance(mapping_begin, region_before.begin);
        // Is it already at this position? Nice -- early out.
        if(byte_index == byte_index_before)
            return false;

        // Oh god we need to move all of the memory to the new index.
        // Note: this will not invalidate mapped terminal pools, so we should be very careful to ensure moved elements move the underlying type to its new memory location too.
        // we're not gonna do any checking here. if something important is going to be in our new memory block -- tough shit
        char* destination_address = reinterpret_cast<char*>(mapping_begin) + byte_index;
        char* source_address = reinterpret_cast<char*>(mapping_begin) + byte_index_before;
        // do the copy.
        std::memcpy(destination_address, source_address, region.block.size());
        // now edit our region to point to this new area.
        region.block = {destination_address, region_size_bytes};
        this->regions[region] = region_name;
        return true;
    }

    template<tz::gl::BufferType Type>
    const char* ManagedBuffer<Type>::region_within(std::size_t byte_index) const
    {
         
    }

    template<tz::gl::BufferType Type>
    typename ManagedBuffer<Type>::MapType::iterator ManagedBuffer<Type>::find_region_iter(const std::string& name)
    {
        for(auto iter = this->regions.begin(); iter != this->regions.end(); iter++)
        {
            if(iter->second == name)
                return iter;
        }
        return this->regions.end();
    }

    template<tz::gl::BufferType Type>
    typename ManagedBuffer<Type>::MapType::const_iterator ManagedBuffer<Type>::find_region_iter(const std::string& name) const
    {
        for(auto iter = this->regions.begin(); iter != this->regions.end(); ++iter)
        {
            if(iter->second == name)
                return iter;
        }
        return this->regions.end();
    }

}