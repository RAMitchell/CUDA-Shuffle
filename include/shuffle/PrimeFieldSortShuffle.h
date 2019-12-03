#pragma once
#include <thrust/device_vector.h>

#include <thrust/copy.h>
#include <thrust/sequence.h>
#include <thrust/sort.h>
#include <thrust/transform.h>

#include "shuffle/Shuffle.h"

template <class ContainerType = thrust::device_vector<uint64_t>, class RandomGenerator = DefaultRandomGenerator>
class PrimeFieldSortShuffle : public Shuffle<ContainerType, RandomGenerator>
{
private:
    static uint64_t roundUpPower2( uint64_t a )
    {
        if( a & ( a - 1 ) )
        {
            uint64_t i;
            for( i = 0; a > 1; i++ )
            {
                a >>= 1ull;
            }
            return 1ull << ( i + 1ull );
        }
        return a;
    }

public:
    void shuffle( const ContainerType& in_container, ContainerType& out_container, uint64_t seed, uint64_t num ) override
    {
        if( &in_container != &out_container )
        {
            // Copy if we are not doing an inplace operation
            thrust::copy( thrust::device, in_container.begin(), in_container.begin() + num,
                          out_container.begin() );
        }

        RandomGenerator random_function( seed );
        // Round up to power of two
        uint64_t cap = roundUpPower2( num );
        // Choose an odd number so we know it is coprime with cap
        uint64_t mul = ( random_function() * 2 + 1 ) % cap;
        // Choose a shift
        uint64_t shift = random_function() % cap;

        thrust::device_vector<uint64_t> keys( num );

        // Initialise key vector with indexes
        thrust::sequence( thrust::device, keys.begin(), keys.end() );
        // Inplace transform
        thrust::transform( thrust::device, keys.begin(), keys.end(), keys.begin(),
                           [=] __host__ __device__( uint64_t val ) -> uint64_t {
                               return ( val * mul + shift ) % cap;
                           } );
        // Sort by keys
        thrust::sort_by_key( thrust::device, keys.begin(), keys.end(), out_container.begin() );
    }
};