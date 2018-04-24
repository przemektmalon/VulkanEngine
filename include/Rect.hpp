#pragma once
#include "PCH.hpp"

template<class T>
class rect
{
public:
	rect() {}
	rect(T pLeft, T pTop, T pWidth, T pHeight) : left(pLeft), top(pTop), width(pWidth), height(pHeight) {}
	template<class R>
	rect(rect<R> pCons) : left(pCons.left), top(pCons.top), width(pCons.width), height(pCons.height) {}

	union
	{
		struct { T left, top, width, height; };
		struct { glm::tvec2<T> topLeft, size; };
		struct { glm::tvec4<T> all; };
	};

	T bot() const { return top + height; }
	T right() const { return left + width; }
	glm::tvec2<T> mid() { return glm::tvec2<T>(left + width * 0.5, top + height * 0.5); }

	template<class R>
	void operator=(const rect<R>& rhs)
	{
		left = (T)rhs.left;
		top = (T)rhs.top;
		width = (T)rhs.width;
		height = (T)rhs.height;
	}

	bool operator==(const rect<T>& rhs)
	{
		return ((top == rhs.top) && (left == rhs.left) && (width == rhs.width) && (height == rhs.height));
	}

	bool operator!=(const rect<T>& rhs)
	{
		return !(this == rhs);
	}

	T& operator[](int index)
	{
		return all[index];
	}

	rect<T> operator+(const rect<T>& rhs)
	{
		T minX1 = min(left, left + width);
		T maxX1 = max(left, left + width);
		T minY1 = min(top, top + height);
		T maxY1 = max(top, top + height);

		T minX2 = min(rhs.left, rhs.left + rhs.width);
		T maxX2 = max(rhs.left, rhs.left + rhs.width);
		T minY2 = min(rhs.top, rhs.top + rhs.height);
		T maxY2 = max(rhs.top, rhs.top + rhs.height);

		T minBot = min(minY1, minY2);
		T minLeft = min(minX1, minX2);
		T maxRight = max(maxX1, maxX2);
		T maxTop = max(maxY1, maxY2);

		return rect<T>(minLeft, minBot, maxRight - minLeft, maxTop - minBot);
	}

	void zero()
	{
		top = 0; left = 0; width = 0; height = 0;
	}

	template<class R>
	bool contains(const glm::tvec2<R>& pPoint)
	{
		return pPoint.x > left && pPoint.x < left + width && pPoint.y > top && pPoint.y < top + height;
	}

	bool intersects(const rect<T>& pTarget, rect<T>& pIntersection)
	{
		T minX1 = std::min(left, left + width);
		T maxX1 = std::max(left, left + width);
		T minY1 = std::min(top, top + height);
		T maxY1 = std::max(top, top + height);

		T minX2 = std::min(pTarget.left, pTarget.left + pTarget.width);
		T maxX2 = std::max(pTarget.left, pTarget.left + pTarget.width);
		T minY2 = std::min(pTarget.top, pTarget.top + pTarget.height);
		T maxY2 = std::max(pTarget.top, pTarget.top + pTarget.height);

		T botIntersect = std::max(minY1, minY2);
		T leftIntersect = std::max(minX1, minX2);
		T rightIntersect = std::min(maxX1, maxX2);
		T topIntersect = std::min(maxY1, maxY2);

		if ((leftIntersect < rightIntersect) && (botIntersect < topIntersect))
		{
			pIntersection = rect<T>(leftIntersect, botIntersect, rightIntersect - leftIntersect, botIntersect - botIntersect);
			return true;
		}
		else
		{
			pIntersection = rect<T>(0, 0, 0, 0);
			return false;
		}
	}

	bool intersects(const rect<T>& pTarget)
	{
		rect<T> intersection;
		return this->intersects(pTarget, intersection);
	}

};

typedef rect<float> frect;
typedef rect<double> drect;
typedef rect<int> irect;