#ifndef IV_LV5_METHOD_TABLE_H_
#define IV_LV5_METHOD_TABLE_H_
namespace iv {
namespace lv5 {

#define IV_LV5_INTERNAL_METHOD static

struct MethodTable {
  typedef JSVal(*GetNonIndexedSlotType)(JSObject* obj, Context* ctx, Symbol name, Slot* slot, Error* e);
  typedef JSVal(*GetIndexedSlotType)(JSObject* obj, Context* ctx, uint32_t index, Slot* slot, Error* e);
  typedef bool(*GetNonIndexedPropertySlotType)(const JSObject* obj, Context* ctx, Symbol name, Slot* slot);
  typedef bool(*GetIndexedPropertySlotType)(const JSObject* obj, Context* ctx, uint32_t index, Slot* slot);
  typedef bool(*GetOwnNonIndexedPropertySlotType)(const JSObject* obj, Context* ctx, Symbol name, Slot* slot);
  typedef bool(*GetOwnIndexedPropertySlotType)(const JSObject* obj, Context* ctx, uint32_t index, Slot* slot);
  typedef void(*PutNonIndexedSlotType)(JSObject* obj, Context* context, Symbol name, JSVal val, Slot* slot, bool th, Error* e);
  typedef void(*PutIndexedSlotType)(JSObject* obj, Context* context, uint32_t index, JSVal val, Slot* slot, bool th, Error* e);
  typedef bool(*DeleteNonIndexedType)(JSObject* obj, Context* ctx, Symbol name, bool th, Error* e);
  typedef bool(*DeleteIndexedType)(JSObject* obj, Context* ctx, uint32_t name, bool th, Error* e);
  typedef bool(*DefineOwnNonIndexedPropertySlotType)(JSObject* obj, Context* ctx, Symbol name, const PropertyDescriptor& desc, Slot* slot, bool th, Error* e);
  typedef bool(*DefineOwnIndexedPropertySlotType)(JSObject* obj, Context* ctx, uint32_t index, const PropertyDescriptor& desc, Slot* slot, bool th, Error* e);
  typedef void(*GetPropertyNamesType)(const JSObject* obj, Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode);
  typedef void(*GetOwnPropertyNamesType)(const JSObject* obj, Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode);
  typedef JSVal(*DefaultValueType)(JSObject* obj, Context* ctx, Hint::Object hint, Error* e);

  GetNonIndexedSlotType GetNonIndexedSlot;
  GetIndexedSlotType GetIndexedSlot;
  GetNonIndexedPropertySlotType GetNonIndexedPropertySlot;
  GetIndexedPropertySlotType GetIndexedPropertySlot;
  GetOwnNonIndexedPropertySlotType GetOwnNonIndexedPropertySlot;
  GetOwnIndexedPropertySlotType GetOwnIndexedPropertySlot;
  PutNonIndexedSlotType PutNonIndexedSlot;
  PutIndexedSlotType PutIndexedSlot;
  DeleteNonIndexedType DeleteNonIndexed;
  DeleteIndexedType DeleteIndexed;
  DefineOwnNonIndexedPropertySlotType DefineOwnNonIndexedPropertySlot;
  DefineOwnIndexedPropertySlotType DefineOwnIndexedPropertySlot;
  GetPropertyNamesType GetPropertyNames;
  GetOwnPropertyNamesType GetOwnPropertyNames;
  DefaultValueType DefaultValue;
};

#define IV_LV5_METHOD_TABLE(Class)\
  {\
    &Class::GetNonIndexedSlotMethod,\
    &Class::GetIndexedSlotMethod,\
    &Class::GetNonIndexedPropertySlotMethod,\
    &Class::GetIndexedPropertySlotMethod,\
    &Class::GetOwnNonIndexedPropertySlotMethod,\
    &Class::GetOwnIndexedPropertySlotMethod,\
    &Class::PutNonIndexedSlotMethod,\
    &Class::PutIndexedSlotMethod,\
    &Class::DeleteNonIndexedMethod,\
    &Class::DeleteIndexedMethod,\
    &Class::DefineOwnNonIndexedPropertySlotMethod,\
    &Class::DefineOwnIndexedPropertySlotMethod,\
    &Class::GetPropertyNamesMethod,\
    &Class::GetOwnPropertyNamesMethod,\
    &Class::DefaultValueMethod\
  }

} }  // namespace iv::lv5
#endif // IV_LV5_METHOD_TABLE_H_
