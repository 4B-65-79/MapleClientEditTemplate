#include "TSecType.h"
#include <Windows.h>

SECPOINT::SECPOINT() { };

SECPOINT::SECPOINT(long ptX, long ptY)
{
    this->x = ptX;
    this->y = ptY;
}

SECPOINT::SECPOINT(SECPOINT* ptSrc)
{
    this->x.SetData(ptSrc->x.GetData());
    this->y.SetData(ptSrc->y.GetData());
}

SECPOINT::SECPOINT(tagPOINT* ptSrc)
{
    this->x.SetData(ptSrc->x);
    this->y.SetData(ptSrc->y);
}

SECPOINT::~SECPOINT()
{
    this->x.~TSecType();
    this->y.~TSecType();
}

SECPOINT* SECPOINT::operator =(tagPOINT* ptSrc)
{
    this->x.SetData(ptSrc->x);
    this->y.SetData(ptSrc->y);
    return this;
}

SECPOINT* SECPOINT::operator =(SECPOINT* ptSrc)
{
    this->x.SetData(ptSrc->x.GetData());
    this->y.SetData(ptSrc->y.GetData());
    return this;
}

long SECPOINT::GetX() {
    return this->x.GetData();
}

long SECPOINT::GetY() {
    return this->y.GetData();
}

bool SECPOINT:: operator !=(tagPOINT* ptSrc)
{
    return this->x.GetData() != ptSrc->x || this->y.GetData() != ptSrc->y;
}

bool SECPOINT:: operator ==(tagPOINT* ptSrc)
{
    return this->x.GetData() == ptSrc->x && this->y.GetData() == ptSrc->y;
}

bool SECPOINT::operator !=(SECPOINT* ptSrc)
{
    return this->x.GetData() != ptSrc->x.GetData() || this->y.GetData() != ptSrc->y.GetData();
}

bool SECPOINT::operator ==(SECPOINT* ptSrc)
{
    return this->x.GetData() == ptSrc->x.GetData() && this->y.GetData() == ptSrc->y.GetData();
}

SECPOINT::operator tagPOINT()
{
    return { this->x.GetData(), this->y.GetData() };
}