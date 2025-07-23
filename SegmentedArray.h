#pragma once

//#include <vector>
//#include <bitset>
#include <stdexcept>
#include <cassert>
//#include <mutex>
#include <iostream>

//template<uint64_t Bits>
class MyBitset
{
private:
	static const uint64_t WORD_2POWER = 6ULL; // 2^6==sizeof(uint64_t)*8 = 64;
	static const uint64_t BITS_IN_WORD = 1ULL << WORD_2POWER; //==sizeof(uint64_t)*8 = 64;
	static const uint64_t WORD_MASK = BITS_IN_WORD - 1ULL; // =63=0x3F

	uint64_t m_bits;
	uint64_t* arr; // [(Bits - 1ULL) / BITS_IN_WORD + 1ULL] ;
	
//	bool get_bit(uint64_t word, uint32_t offset)
//	{
//		assert(offset < WORD_IN_BITS);
//		uint64_t tmp = (word >> offset) & 0x01;
//		//tmp &= 0x01;
//		return tmp == 1;
//	}

	//uint64_t set_bit(uint64_t word, uint32_t offset, bool bit)
	//{
	//	assert(offset < BITS_IN_WORD);
	//	uint64_t mask = bit ? 0x01 : 0x00;
	//	//mask <<= offset;

	//	return word | (mask << offset); // не сработает правильно если ранее туда записана 1 и мы сейчас хотим заисать 0.
	//}

public:
	MyBitset(uint64_t BitsCount) 
	{ 
		m_bits = BitsCount;
		uint64_t wordsCnt = (BitsCount - 1ULL) / BITS_IN_WORD + 1ULL;
		arr = (uint64_t*)calloc(wordsCnt, sizeof(uint64_t));
		//arr = new uint64_t[wordsCnt];
		//memset(arr, 0, wordsCnt);
	}

	~MyBitset()
	{
		//delete[] arr;
		free(arr);
	}

	inline bool get(uint64_t bitIndex) const
	{
		uint64_t index2 = bitIndex >> WORD_2POWER; // index/BITS_IN_WORD;
		uint64_t offset = bitIndex & WORD_MASK; // index % BITS_IN_WORD;

		return ((arr[index2] >> offset) & 0x01) == 1ULL;
	}

	inline void setTrue(uint64_t bitTndex)
	{
		uint64_t index2 = bitTndex >> WORD_2POWER; // index/BITS_IN_WORD;
		uint64_t offset = bitTndex & WORD_MASK; // index % BITS_IN_WORD;

		arr[index2] |= (1ull << offset); //TODO не сработает правильно если ранее туда записана 1 и мы сейчас хотим записать 0.
		//arr[index2] = set_bit(arr[index2], offset, value);
	} 
};

// This array historically called segmented array just implements bitset with index shift.
// It stores values with index m_begin to m_end and stores only (m_end - m_begin) values
class SegmentedArray
{
private:
	//typedef MyBitset mybitset_t;
	//typedef mybitset_t* pbitset;

	MyBitset* m_data;
	uint64_t m_begin;
	//uint64_t m_size;
	uint64_t m_end;

public:
	SegmentedArray(uint64_t start, uint64_t end)
	{
		assert(end > start);
		m_data  = new MyBitset(end - start); // length is bits count here
		m_begin = start;
		//m_size  = length;
		m_end   = end;
	}

	~SegmentedArray()
	{
		delete m_data;
	}

	inline void setTrue(uint64_t index)
	{
		assert(index >= m_begin);
		assert(index < m_end);
		m_data->setTrue(index - m_begin);
	}

	inline bool get(uint64_t index) const
	{
		assert(index >= m_begin);
		assert(index < m_end);
		uint64_t index2 = index - m_begin;
		return m_data->get(index2);
	}

	uint64_t end() const { return m_end; }
};


// TODO may be we dont need segmented array. Check if it is possible to index and allocate arrays greater than 4G in C++.
/*class SegmentedArrayOLD
{
private:
	//static const uint64_t SEG_2POWER = 37ULL; // 128G maximum segment size to cover usual Length=100G //30;
	//static const uint64_t SEGMENT_SIZE = 1ULL << SEG_2POWER; //128G  //1_073_741_824; //2^30 - что бы можно было эффективно делать деление сдвигом вправо на 30 бит
	//static const uint64_t OFFSET_MASK = SEGMENT_SIZE - 1ULL; // 0x3FFFFFFF

//#define VECTOR_BOOL
//#define	STD_BITSET
#define	MY_BITSET
//#define CHECK_ARRAY_BOUNDS

#ifdef STD_BITSET
	typedef bitset<SEGMENT_SIZE> mybitset_t;
#endif // STD_BITSET

#ifdef VECTOR_BOOL
	typedef vector<bool> mybitset_t;
#endif // VECTOR_BOOL

#ifdef MY_BITSET
	typedef MyBitset mybitset_t;
#endif // MY_BITSET

	typedef mybitset_t* pbitset;
	pbitset* m_segments; // NOTE, this is array of pointers to segments
 
	uint64_t m_begin;
	uint64_t m_cpacity;
	uint64_t m_end;
	uint64_t m_numOfSeg;

public:
	SegmentedArrayOLD(uint64_t start, uint64_t length)
	{
		m_numOfSeg = length / length;//SEGMENT_SIZE;
		
		uint64_t remaining = (length - (m_numOfSeg * length));
		
		if (remaining > 0) m_numOfSeg++;
		
		m_segments = new pbitset[m_numOfSeg]; //array of pointers to segments

		for (uint64_t i = 0; i < m_numOfSeg; ++i)
		{
			m_segments[i] = new mybitset_t(length);
		}

		m_begin = start;
		m_cpacity = length;
		m_end = m_begin + m_cpacity;
	}


	~SegmentedArrayOLD()
	{
		for (uint64_t i = 0; i < m_numOfSeg; ++i)
		{
			delete m_segments[i];
		}

		delete[] m_segments;
	}

	void set(uint64_t index, bool value)
	{
#ifdef CHECK_ARRAY_BOUNDS
		if (index >= m_end) throw invalid_argument("Index out of bounds");
		if (index < m_begin) throw invalid_argument("Index out of bounds"); //return; //do nothing with indexes from 0...m_begin. Emulate that those values are present in an array
#endif
		uint64_t index2 = index - m_begin;
		//uint64_t segNo = index2 >> SEG_2POWER; // делим на SEGMENT_SIZE который есть 2^35 //30
		//uint64_t offset = index2 & OFFSET_MASK; // 0x3fffffff(еще больше f здесь)

		pbitset pb = m_segments[0/*segNo*//*];
		assert(pb != nullptr);

		pb->set(index2/*offset*//*, value);
	}

	bool get(uint64_t index)
	{
#ifdef CHECK_ARRAY_BOUNDS
		if (index >= m_end) throw invalid_argument("Index out of bounds");
		if (index < m_begin) throw invalid_argument("Index out of bounds");  //return false; //do nothing with indexes from 0...m_begin. Emulate that those values are present in an array
#endif

		uint64_t index2 = index - m_begin;
		//uint64_t segNo = index2 >> SEG_2POWER; // делим на SEGMENT_SIZE который есть 2^30
		//uint64_t offset = index2 & OFFSET_MASK; // 0x3fffffff
		
		pbitset pb = m_segments[0/*segNo*//*];
		assert(pb != nullptr);

#ifdef STD_BITSET
		return pb->test(offset);
#else
		return pb->get(index2/*offset*//*);
#endif
	}

	uint64_t size() const { return m_cpacity; }

	//uint64_t capacity() { return m_cpacity; }

	//uint64_t segm() { return m_segments->size(); }

};*/

