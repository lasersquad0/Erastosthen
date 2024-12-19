#pragma once

#include <vector>
#include <bitset>
#include <stdexcept>
#include <cassert>
#include <mutex>
#include <iostream>


using namespace std;

//template<uint64_t Bits>
class MyBitset
{
private:
	static const uint64_t WORD_2POWER = 6ULL; // 2^6==sizeof(uint64_t)*8 = 64;
	static const uint64_t BITS_IN_WORD = 1ULL << WORD_2POWER; //==sizeof(uint64_t)*8 = 64;
	static const uint64_t WORD_MASK = BITS_IN_WORD - 1ULL; // =63=0x3F

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
		arr = new uint64_t[(BitsCount - 1ULL) / BITS_IN_WORD + 1ULL];
	}

	~MyBitset()
	{
		delete[] arr;
	}

	bool get(uint64_t index)
	{
		uint64_t index2 = index >> WORD_2POWER; // index/BITS_IN_WORD;
		uint64_t offset = index & WORD_MASK; // index % BITS_IN_WORD;

		return ((arr[index2] >> offset) & 0x01) == 1ULL;
	}

	void set(uint64_t index, bool value)
	{
		assert(value); // only true is allowed

		uint64_t index2 = index >> WORD_2POWER; // index/BITS_IN_WORD;
		uint64_t offset = index & WORD_MASK; // index % BITS_IN_WORD;

		uint64_t mask = (uint64_t)value; // ? 0x01 : 0x00;

		arr[index2] |= (mask << offset); // не сработает правильно если ранее туда записана 1 и мы сейчас хотим записать 0.
		//arr[index2] = set_bit(arr[index2], offset, value);
	}
	 
};

// TODO may be we dont need segmented array. Check if it is possible to index and allocate arrays greater than 4G in C++.
class SegmentedArray
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
	typedef MyBitset/*<SEGMENT_SIZE>*/ mybitset_t;
#endif // MY_BITSET

	typedef mybitset_t* pbitset;
	pbitset* m_segments;

	uint64_t m_begin;
	uint64_t m_cpacity;
	uint64_t m_sz;
	uint64_t m_numOfSeg;

public:
	SegmentedArray(uint64_t start, uint64_t length)
	{
		m_numOfSeg = length / length;//SEGMENT_SIZE;
		
		uint64_t remaining = (length - (m_numOfSeg * length/*SEGMENT_SIZE*/));
		
		if (remaining > 0) m_numOfSeg++;
		
		m_segments = new pbitset[m_numOfSeg];

		for (uint64_t i = 0; i < m_numOfSeg; ++i)
		{
			m_segments[i] = new mybitset_t(length);
		}

		m_begin = start;
		m_cpacity = length;
		m_sz = m_begin + m_cpacity;
	}


	~SegmentedArray()
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
		if (index >= m_sz) throw invalid_argument("Index out of bounds");
		if (index < m_begin) throw invalid_argument("Index out of bounds"); //return; //do nothing with indexes from 0...m_begin. Emulate that those values are present in an array
#endif
		uint64_t index2 = index - m_begin;
		//uint64_t segNo = index2 >> SEG_2POWER; // делим на SEGMENT_SIZE который есть 2^35 //30
		//uint64_t offset = index2 & OFFSET_MASK; // 0x3fffffff(еще больше f здесь)

		pbitset pb = m_segments[0/*segNo*/];
		assert(pb != nullptr);

		pb->set(index2/*offset*/, value);
	}

	bool get(uint64_t index)
	{
#ifdef CHECK_ARRAY_BOUNDS
		if (index >= m_sz) throw invalid_argument("Index out of bounds");
		if (index < m_begin) throw invalid_argument("Index out of bounds");  //return false; //do nothing with indexes from 0...m_begin. Emulate that those values are present in an array
#endif

		uint64_t index2 = index - m_begin;
		//uint64_t segNo = index2 >> SEG_2POWER; // делим на SEGMENT_SIZE который есть 2^30
		//uint64_t offset = index2 & OFFSET_MASK; // 0x3fffffff
		
		pbitset pb = m_segments[0/*segNo*/];
		assert(pb != nullptr);

#ifdef STD_BITSET
		return pb->test(offset);
#else
		return pb->get(index2/*offset*/);
#endif
	}

	uint64_t size() { return m_sz; }

	//uint64_t capacity() { return m_cpacity; }

	//uint64_t segm() { return m_segments->size(); }

};

