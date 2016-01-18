#include "sgClassMetaDef.h"
#include "aa.h"
#include <map>

namespace Hehe {
	void Func1();
	SG_META_NO_EXPORT void Func2();

	enum SG_META_ENUM RoomType
	{
		RT_1,
		RT_2
	};

	class Person
	{
	};


	class Room : public PP
	{
		SG_META_OBJECT(Room)
	public:
		SG_META_NO_EXPORT int aa;
		float ff;
		const char *cc;
		bool bb2;
		int arr[12];

		SG_META_NO_EXPORT void add_person(Person person)
		{

		}


		class ABC : public Person {};
		ABC abc;

		enum EnumAA
		{
			AA_A,
			AA_B = 3,
			AA_C
		};

		EnumAA eaa;

	private:
		Person* people_in_room;
	};



}
