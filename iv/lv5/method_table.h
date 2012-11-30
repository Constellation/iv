#ifndef IV_LV5_METHOD_TABLE_H_
#define IV_LV5_METHOD_TABLE_H_
namespace iv {
namespace lv5 {

struct MethodTable {
  typedef JSVal(*GetSlotType)(JSObject* obj, Context* ctx, Symbol name, Slot* slot, Error* e);
  typedef JSVal(*GetPropertySlotType)(JSObject* obj, Context* ctx, Symbol name, Slot* slot, Error* e);
  typedef bool(*GetOwnPropertySlotType)(const JSObject* obj, Context* ctx, Symbol name, Slot* slot);
  typedef bool(*GetOwnIndexedPropertySlotType)(const JSObject* obj, Context* ctx, Symbol name, Slot* slot);
  typedef void(*PutSlotType)(JSObject* obj, Context* context, Symbol name, JSVal val, Slot* slot, bool th, Error* e);
  typedef bool(*DeleteType)(JSObject* obj, Context* ctx, Symbol name, bool th, Error* e);
  typedef bool(*DefineOwnPropertySlotType)(JSObject* obj, Context* ctx, Symbol name, const PropertyDescriptor& desc, Slot* slot, bool th, Error* e);
  typedef void(*GetPropertyNamesType)(const JSObject* obj, Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode);
  typedef void(*GetOwnPropertyNamesType)(const JSObject* obj, Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode);

  GetSlotType GetSlot;
  GetPropertySlotType GetPropertySlot;
  GetOwnPropertySlotType GetOwnPropertySlot;
  PutSlotType PutSlot;
  DeleteType Delete;
  DefineOwnPropertySlotType DefineOwnPropertySlot;
  GetPropertyNamesType GetPropertyNames;
  GetOwnPropertyNamesType GetOwnPropertyNames;
};

#define IV_LV5_METHOD_TABLE(Class)\
  {\
    &Class::GetSlot,\
    &Class::GetPropertySlot,\
    &Class::GetOwnPropertySlot,\
    &Class::GetOwnIndexedPropertySlot,\
    &Class::PutSlot,\
    &Class::Delete,\
    &Class::DefineOwnPropertySlot,\
    &Class::GetPropertyNames,\
    &Class::GetOwnPropertyNames\
  }

} }  // namespace iv::lv5
#endif // IV_LV5_METHOD_TABLE_H_
