
 
static const char Injected[] = "\
							   #define SG_ANNOTATE_CLASS(type, anotation) __extension__ _Static_assert(sizeof (#anotation), #type);\n	\
							   #define SG_ANNOTATE_CLASS2(type, a1, a2) __extension__ _Static_assert(sizeof (#a1, #a2), #type); \n	\
							   \
							   #undef  SG_META_OBJ_ABSTRACT \n \
							   #define SG_META_OBJ_ABSTRACT(x) SG_ANNOTATE_CLASS(sg_meta_object_abstract, x)	\n \
							   \
							   #undef  SG_META_OBJECT \n \
							   #define SG_META_OBJECT(x) SG_ANNOTATE_CLASS(sg_meta_object, x)	\n \
							   \
							   #undef  SG_META_OTHER \n \
							   #define SG_META_OTHER(x) SG_ANNOTATE_CLASS(sg_meta_other, x)	\n \
							   \
							   #undef  SG_META_ENUM \n \
							   #define SG_META_ENUM __attribute__((annotate(\"sg_meta_enum\")))	\n \
							   \
							   #undef  SG_META_NO_EXPORT \n \
							   #define SG_META_NO_EXPORT __attribute__((annotate(\"sg_meta_no_export\")))		\
";
