#pragma once

#include <vector>
#include <bitset>
#include <stdexcept>
#include <cassert>
#include <mutex>

using namespace std;

template<uint32_t Bits>
class MyBitset
{
private:
	static const uint32_t WORD_2POWER = 6; // 2^6==sizeof(uint64_t)*8 = 64;
	static const uint32_t BITS_IN_WORD = 1 << WORD_2POWER; //==sizeof(uint64_t)*8 = 64;
	static const uint32_t WORD_MASK = BITS_IN_WORD - 1; // =63=0x3F

	uint64_t arr[(Bits-1)/BITS_IN_WORD + 1];
	//mutex mtx;

//	bool get_bit(uint64_t word, uint32_t offset)
//	{
//		assert(offset < WORD_IN_BITS);
//		uint64_t tmp = (word >> offset) & 0x01;
//		//tmp &= 0x01;
//		return tmp == 1;
//	}

	uint64_t set_bit(uint64_t word, uint32_t offset, bool bit)
	{
		assert(offset < BITS_IN_WORD);
		uint64_t mask = bit ? 0x01 : 0x00;
		//mask <<= offset;

		return word | (mask << offset); // не сработает правильно если ранее туда записана 1 и мы сейчас хотим заисать 0.
	}

public:
	bool get(uint32_t index)
	{
		uint32_t index2 = index >> WORD_2POWER; //index / BITS_IN_WORD;
		uint32_t offset = index & WORD_MASK; //index % BITS_IN_WORD;

		return ((arr[index2] >> offset) & 0x01) == 1;
	}

	void set(uint32_t index, bool value)
	{
		assert(value); // only true is allowed

		uint32_t index2 = index >> WORD_2POWER; //index / BITS_IN_WORD;
		uint32_t offset = index & WORD_MASK; //index % BITS_IN_WORD;

		uint64_t mask = (uint64_t)value;// ? 0x01 : 0x00;

		arr[index2] |= (mask << offset); // не сработает правильно если ранее туда записана 1 и мы сейчас хотим заисать 0.

		//mtx.lock();
		//arr[index2] = set_bit(arr[index2], offset, value);
		//mtx.unlock();
	}
	 
};

class SegmentedArray
{
private:
	static const uint32_t SEG_2POWER = 30;
	static const uint32_t SEGMENT_SIZE = 1 << SEG_2POWER; //1_073_741_824; //2^30 - что бы можно было эффективно делать деление сдвигом вправо на 30 бит
	static const uint32_t OFFSET_MASK = SEGMENT_SIZE - 1; // 0x3FFFFFFF

//#define VECTOR_BOOL
//#define	STD_BITSET
#define	MY_BITSET

#ifdef STD_BITSET
	typedef bitset<SEGMENT_SIZE> mybitset;
#endif // STD_BITSET

#ifdef VECTOR_BOOL
	typedef vector<bool> mybitset;
#endif // VECTOR_BOOL

#ifdef MY_BITSET
	typedef MyBitset<SEGMENT_SIZE> mybitset;
#endif // MY_BITSET

	typedef mybitset* pbitset;
	vector<pbitset>* segments;

	uint64_t begin;
	uint64_t cpacity;
	uint64_t sz;

	void addSegment()
	{
		auto seg = new mybitset();
#ifdef VECTOR_BOOL
		seg->assign(SEGMENT_SIZE, false);
#endif
		segments->push_back(seg);
	}

public:
	SegmentedArray(uint64_t start, uint64_t length)
	{
		const uint32_t numOfSeg = (uint32_t)(length / (uint64_t)SEGMENT_SIZE);
		segments = new vector<pbitset>(); // (numOfSeg);

		for (uint32_t i = 0; i < numOfSeg; ++i)
		{
			addSegment();
		}

		int remaining = (uint32_t)(length - ((uint64_t)numOfSeg * SEGMENT_SIZE));
		if (remaining > 0)
		{
			addSegment();
		}

		begin = start;
		cpacity = length;
		sz = begin + cpacity;
	}

	~SegmentedArray()
	{
		for (uint32_t i = 0; i < segments->size(); ++i)
		{
			delete segments->at(i);
		}

		delete segments;
	}

	void set(uint64_t index, bool value)
	{
		if (index >= sz) throw invalid_argument("Index out of bounds");

		if (index < begin) throw invalid_argument("Index out of bounds"); //return; //do nothing with indexes from 0...begin. Emulate that those values are present in an array

		uint64_t index2 = index - begin;
		uint32_t segNo = (uint32_t)(index2 >> SEG_2POWER); // делим на SEGMENT_SIZE который есть 2^30
		uint32_t offset = ((uint32_t)(index2) & OFFSET_MASK); //0x3fffffff

		pbitset pb = segments->at(segNo);
		//assert(pb != NULL);
		pb->set(offset, value);
		
	}

	bool get(uint64_t index)
	{
		if (index >= sz) throw invalid_argument("Index out of bounds");

		if (index < begin) throw invalid_argument("Index out of bounds");  //return false; //do nothing with indexes from 0...begin. Emulate that those values are present in an array

		uint64_t index2 = index - begin;
		uint32_t segNo = (uint32_t)(index2 >> SEG_2POWER); // делим на SEGMENT_SIZE который есть 2^30
		uint32_t offset = ((uint32_t)(index2) & OFFSET_MASK); // 0x3fffffff
		
		pbitset pb = segments->at(segNo);
		return pb->get(offset);
	}

	uint64_t size() { return sz; }

	//uint64_t capacity() { return cpacity; }

	//uint64_t segm() { return segments->size(); }

};

