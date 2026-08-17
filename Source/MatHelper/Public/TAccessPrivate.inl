// Copyright AKaKLya 2024
// UE4.26 port: no C++17 inline static; use function-local static instead.

#pragma once

//----------[Call Private Variable]-------------//
template <class T>
struct TAccessPrivate
{
	static typename T::Type& Get()
	{
		static typename T::Type Value;
		return Value;
	}
};

template <class T, typename T::Type Value>
struct TAccessPrivateStub
{
	struct FAccessPrivateStub
	{
		FAccessPrivateStub()
		{
			TAccessPrivate<T>::Get() = Value;
		}
	};

	static FAccessPrivateStub& GetStub()
	{
		static FAccessPrivateStub AccessPrivateStub;
		return AccessPrivateStub;
	}
};

//----------[Call Private Function]-------------//

template <typename T, typename FuncPtr>
struct TAccessPrivateFunction
{
	static FuncPtr& Get()
	{
		static FuncPtr Value;
		return Value;
	}
};

template <typename T, typename FuncPtr, FuncPtr Value>
struct TAccessPrivateFunctionStub
{
	struct FAccessPrivateStub
	{
		FAccessPrivateStub()
		{
			TAccessPrivateFunction<T, FuncPtr>::Get() = Value;
		}
	};

	static FAccessPrivateStub& GetStub()
	{
		static FAccessPrivateStub AccessPrivateStub;
		return AccessPrivateStub;
	}
};
