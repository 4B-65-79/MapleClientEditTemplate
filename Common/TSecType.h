#pragma once
#include "asserts.h"

#include <Windows.h>

/*
	Original Credits: https://github.com/67-6f-64/Firefly/blob/master/Firefly%20Spy/TSecType.hpp
	Modifications Made By:
		- Rajan Grewal
		- Minimum Delta

	Additional Information From: https://en.cppreference.com/w/cpp/language/operators
*/

// defining LOBYTE instead of include Windows.h to reduce compilation time
#ifndef LOBYTE
#define LOBYTE(w)           ((unsigned char)(((unsigned int)(w)) & 0xff))
#endif

typedef struct tagPOINT;

template <typename T>
class TSecData
{
public:
	T data;
	unsigned char  bKey;
	unsigned char  FakePtr1;
	unsigned char  FakePtr2;
	unsigned short wChecksum;
};

template <typename T>
class TSecType
{
private:
	unsigned long FakePtr1;
	unsigned long FakePtr2;
	TSecData<T>* m_secdata;

public:
	TSecType()
	{
		this->m_secdata = new TSecData<T>(); // uses proper ZAllocEx now (since global new operator overload)

		this->FakePtr1 = static_cast<unsigned long>(rand());
		this->FakePtr2 = static_cast<unsigned long>(rand());

		this->m_secdata->FakePtr1 = LOBYTE(this->FakePtr1);
		this->m_secdata->FakePtr2 = LOBYTE(this->FakePtr2);

		this->SetData(NULL);
	}

	TSecType(const T op)
	{
		this->m_secdata = new TSecData<T>(); // uses proper ZAllocEx now (since global new operator overload)

		this->FakePtr1 = static_cast<unsigned long>(rand());
		this->FakePtr2 = static_cast<unsigned long>(rand());

		this->m_secdata->FakePtr1 = LOBYTE(this->FakePtr1);
		this->m_secdata->FakePtr2 = LOBYTE(this->FakePtr2);

		this->SetData(op);
	}

	~TSecType()
	{
		if (this->m_secdata)
		{
			delete this->m_secdata;
		}
	}

	operator T()
	{
		return this->GetData();
	}

	BOOL operator ==(TSecType<T>* op)
	{
		return this->GetData() == op->GetData();
	}

	TSecType<T>* operator =(const T op)
	{
		this->SetData(op);
		return this;
	}

	TSecType<T>* operator =(TSecType<T>* op)
	{
		T data = op->GetData();
		this->SetData(data);
		return this;
	}

	T operator /=(const T op)
	{
		T tmp = this->GetData() / op;
		this->SetData(tmp);
		return tmp;
	}

	T operator *=(const T op)
	{
		T tmp = this->GetData() * op;
		this->SetData(tmp);
		return tmp;
	}

	T operator +=(const T op)
	{
		T tmp = this->GetData() + op;
		this->SetData(tmp);
		return tmp;
	}

	T operator -=(const T op)
	{
		T tmp = this->GetData() - op;
		this->SetData(tmp);
		return tmp;
	}

	T GetData()
	{
		T decrypted_data = this->m_secdata->data;
		unsigned short wChecksum = 0;

		for (unsigned char i = 0, key = this->m_secdata->bKey; i < (sizeof(T) + 1); i++)
		{
			if (i > 0)
			{
				key = reinterpret_cast<unsigned char*>(&this->m_secdata->data)[i - 1] + key + 42;;
				wChecksum = i > 1 ? ((8 * wChecksum) | (key + (wChecksum >> 13))) : ((key + 4) | 0xD328);
			}

			if (i < sizeof(T))
			{
				if (!key)
				{
					key = 42;
				}

				reinterpret_cast<unsigned char*>(&decrypted_data)[i] = reinterpret_cast<unsigned char*>(&this->m_secdata->data)[i] ^ key;
			}
		}

		if (this->m_secdata->wChecksum != wChecksum || LOBYTE(this->FakePtr1) != this->m_secdata->FakePtr1 || LOBYTE(this->FakePtr2) != this->m_secdata->FakePtr2)
		{
			return NULL; //TODO: CxxThrow
		}

		return decrypted_data;
	}

	VOID SetData(T data)
	{
		this->m_secdata->bKey = LOBYTE(rand());
		this->m_secdata->wChecksum = sizeof(T) > 1 ? static_cast<unsigned short>(39525) : static_cast<unsigned short>(-26011);

		for (unsigned char i = 0, key = this->m_secdata->bKey; i < (sizeof(T) + 1); i++)
		{
			if (i > 0)
			{
				key = (key ^ reinterpret_cast<unsigned char*>(&data)[i - 1]) + key + 42;
				this->m_secdata->wChecksum = (8 * this->m_secdata->wChecksum) | (key + (this->m_secdata->wChecksum >> 13));
			}

			if (i < sizeof(T))
			{
				if (!key)
				{
					key = 42;
				}

				reinterpret_cast<unsigned char*>(&this->m_secdata->data)[i] = reinterpret_cast<unsigned char*>(&data)[i] ^ key;
			}
		}
	}
};

class SECPOINT
{
private:
	TSecType<long> y;
	TSecType<long> x;

public:
	SECPOINT();
	SECPOINT(long ptX, long ptY);
	SECPOINT(SECPOINT* ptSrc);
	SECPOINT(tagPOINT* ptSrc);

	~SECPOINT();

	long GetX();
	long GetY();

	SECPOINT* operator =(tagPOINT* ptSrc);
	SECPOINT* operator =(SECPOINT* ptSrc);

	bool operator !=(tagPOINT* ptSrc);
	bool operator ==(tagPOINT* ptSrc);
	bool operator !=(SECPOINT* ptSrc);
	bool operator ==(SECPOINT* ptSrc);

	operator tagPOINT();
};

assert_size(sizeof(TSecData<long>), 0x0C)
assert_size(sizeof(TSecType<long>), 0x0C)