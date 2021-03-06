/* valaenumregisterfunction.c generated by valac, the Vala compiler
 * generated from valaenumregisterfunction.vala, do not modify */

/* valaenumregisterfunction.vala
 *
 * Copyright (C) 2008  Jürg Billeter
 * Copyright (C) 2010  Marc-Andre Lureau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 * Author:
 * 	Jürg Billeter <j@bitron.ch>
 */

#include <glib.h>
#include <glib-object.h>
#include <vala.h>
#include <stdlib.h>
#include <string.h>
#include <valaccode.h>


#define VALA_TYPE_TYPEREGISTER_FUNCTION (vala_typeregister_function_get_type ())
#define VALA_TYPEREGISTER_FUNCTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), VALA_TYPE_TYPEREGISTER_FUNCTION, ValaTypeRegisterFunction))
#define VALA_TYPEREGISTER_FUNCTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), VALA_TYPE_TYPEREGISTER_FUNCTION, ValaTypeRegisterFunctionClass))
#define VALA_IS_TYPEREGISTER_FUNCTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VALA_TYPE_TYPEREGISTER_FUNCTION))
#define VALA_IS_TYPEREGISTER_FUNCTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VALA_TYPE_TYPEREGISTER_FUNCTION))
#define VALA_TYPEREGISTER_FUNCTION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), VALA_TYPE_TYPEREGISTER_FUNCTION, ValaTypeRegisterFunctionClass))

typedef struct _ValaTypeRegisterFunction ValaTypeRegisterFunction;
typedef struct _ValaTypeRegisterFunctionClass ValaTypeRegisterFunctionClass;
typedef struct _ValaTypeRegisterFunctionPrivate ValaTypeRegisterFunctionPrivate;

#define VALA_TYPE_ENUM_REGISTER_FUNCTION (vala_enum_register_function_get_type ())
#define VALA_ENUM_REGISTER_FUNCTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), VALA_TYPE_ENUM_REGISTER_FUNCTION, ValaEnumRegisterFunction))
#define VALA_ENUM_REGISTER_FUNCTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), VALA_TYPE_ENUM_REGISTER_FUNCTION, ValaEnumRegisterFunctionClass))
#define VALA_IS_ENUM_REGISTER_FUNCTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VALA_TYPE_ENUM_REGISTER_FUNCTION))
#define VALA_IS_ENUM_REGISTER_FUNCTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VALA_TYPE_ENUM_REGISTER_FUNCTION))
#define VALA_ENUM_REGISTER_FUNCTION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), VALA_TYPE_ENUM_REGISTER_FUNCTION, ValaEnumRegisterFunctionClass))

typedef struct _ValaEnumRegisterFunction ValaEnumRegisterFunction;
typedef struct _ValaEnumRegisterFunctionClass ValaEnumRegisterFunctionClass;
typedef struct _ValaEnumRegisterFunctionPrivate ValaEnumRegisterFunctionPrivate;

struct _ValaTypeRegisterFunction {
	GTypeInstance parent_instance;
	volatile int ref_count;
	ValaTypeRegisterFunctionPrivate * priv;
};

struct _ValaTypeRegisterFunctionClass {
	GTypeClass parent_class;
	void (*finalize) (ValaTypeRegisterFunction *self);
	ValaTypeSymbol* (*get_type_declaration) (ValaTypeRegisterFunction* self);
	gchar* (*get_type_struct_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_base_init_func_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_class_finalize_func_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_base_finalize_func_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_class_init_func_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_instance_struct_size) (ValaTypeRegisterFunction* self);
	gchar* (*get_instance_init_func_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_parent_type_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_gtype_value_table_init_function_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_gtype_value_table_peek_pointer_function_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_gtype_value_table_free_function_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_gtype_value_table_copy_function_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_gtype_value_table_lcopy_value_function_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_gtype_value_table_collect_value_function_name) (ValaTypeRegisterFunction* self);
	gchar* (*get_type_flags) (ValaTypeRegisterFunction* self);
	ValaCCodeFragment* (*get_type_interface_init_declaration) (ValaTypeRegisterFunction* self);
	void (*get_type_interface_init_statements) (ValaTypeRegisterFunction* self, ValaCCodeBlock* block, gboolean plugin);
	ValaSymbolAccessibility (*get_accessibility) (ValaTypeRegisterFunction* self);
};

struct _ValaEnumRegisterFunction {
	ValaTypeRegisterFunction parent_instance;
	ValaEnumRegisterFunctionPrivate * priv;
};

struct _ValaEnumRegisterFunctionClass {
	ValaTypeRegisterFunctionClass parent_class;
};

struct _ValaEnumRegisterFunctionPrivate {
	ValaEnum* _enum_reference;
};


static gpointer vala_enum_register_function_parent_class = NULL;

gpointer vala_typeregister_function_ref (gpointer instance);
void vala_typeregister_function_unref (gpointer instance);
GParamSpec* vala_param_spec_typeregister_function (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags);
void vala_value_set_typeregister_function (GValue* value, gpointer v_object);
void vala_value_take_typeregister_function (GValue* value, gpointer v_object);
gpointer vala_value_get_typeregister_function (const GValue* value);
GType vala_typeregister_function_get_type (void) G_GNUC_CONST;
GType vala_enum_register_function_get_type (void) G_GNUC_CONST;
#define VALA_ENUM_REGISTER_FUNCTION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), VALA_TYPE_ENUM_REGISTER_FUNCTION, ValaEnumRegisterFunctionPrivate))
enum  {
	VALA_ENUM_REGISTER_FUNCTION_DUMMY_PROPERTY
};
ValaEnumRegisterFunction* vala_enum_register_function_new (ValaEnum* en, ValaCodeContext* context);
ValaEnumRegisterFunction* vala_enum_register_function_construct (GType object_type, ValaEnum* en, ValaCodeContext* context);
ValaTypeRegisterFunction* vala_typeregister_function_construct (GType object_type);
void vala_enum_register_function_set_enum_reference (ValaEnumRegisterFunction* self, ValaEnum* value);
void vala_typeregister_function_set_context (ValaTypeRegisterFunction* self, ValaCodeContext* value);
static ValaTypeSymbol* vala_enum_register_function_real_get_type_declaration (ValaTypeRegisterFunction* base);
ValaEnum* vala_enum_register_function_get_enum_reference (ValaEnumRegisterFunction* self);
static ValaSymbolAccessibility vala_enum_register_function_real_get_accessibility (ValaTypeRegisterFunction* base);
static void vala_enum_register_function_finalize (ValaTypeRegisterFunction* obj);


/**
 * Creates a new C function to register the specified enum at runtime.
 *
 * @param en an enum
 * @return   newly created enum register function
 */
ValaEnumRegisterFunction* vala_enum_register_function_construct (GType object_type, ValaEnum* en, ValaCodeContext* context) {
	ValaEnumRegisterFunction* self = NULL;
	ValaEnum* _tmp0_ = NULL;
	ValaCodeContext* _tmp1_ = NULL;
	g_return_val_if_fail (en != NULL, NULL);
	g_return_val_if_fail (context != NULL, NULL);
	self = (ValaEnumRegisterFunction*) vala_typeregister_function_construct (object_type);
	_tmp0_ = en;
	vala_enum_register_function_set_enum_reference (self, _tmp0_);
	_tmp1_ = context;
	vala_typeregister_function_set_context ((ValaTypeRegisterFunction*) self, _tmp1_);
	return self;
}


ValaEnumRegisterFunction* vala_enum_register_function_new (ValaEnum* en, ValaCodeContext* context) {
	return vala_enum_register_function_construct (VALA_TYPE_ENUM_REGISTER_FUNCTION, en, context);
}


static gpointer _vala_code_node_ref0 (gpointer self) {
	return self ? vala_code_node_ref (self) : NULL;
}


static ValaTypeSymbol* vala_enum_register_function_real_get_type_declaration (ValaTypeRegisterFunction* base) {
	ValaEnumRegisterFunction * self;
	ValaTypeSymbol* result = NULL;
	ValaEnum* _tmp0_ = NULL;
	ValaTypeSymbol* _tmp1_ = NULL;
	self = (ValaEnumRegisterFunction*) base;
	_tmp0_ = self->priv->_enum_reference;
	_tmp1_ = _vala_code_node_ref0 ((ValaTypeSymbol*) _tmp0_);
	result = _tmp1_;
	return result;
}


static ValaSymbolAccessibility vala_enum_register_function_real_get_accessibility (ValaTypeRegisterFunction* base) {
	ValaEnumRegisterFunction * self;
	ValaSymbolAccessibility result = 0;
	ValaEnum* _tmp0_ = NULL;
	ValaSymbolAccessibility _tmp1_ = 0;
	ValaSymbolAccessibility _tmp2_ = 0;
	self = (ValaEnumRegisterFunction*) base;
	_tmp0_ = self->priv->_enum_reference;
	_tmp1_ = vala_symbol_get_access ((ValaSymbol*) _tmp0_);
	_tmp2_ = _tmp1_;
	result = _tmp2_;
	return result;
}


ValaEnum* vala_enum_register_function_get_enum_reference (ValaEnumRegisterFunction* self) {
	ValaEnum* result;
	ValaEnum* _tmp0_ = NULL;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = self->priv->_enum_reference;
	result = _tmp0_;
	return result;
}


void vala_enum_register_function_set_enum_reference (ValaEnumRegisterFunction* self, ValaEnum* value) {
	ValaEnum* _tmp0_ = NULL;
	g_return_if_fail (self != NULL);
	_tmp0_ = value;
	self->priv->_enum_reference = _tmp0_;
}


static void vala_enum_register_function_class_init (ValaEnumRegisterFunctionClass * klass) {
	vala_enum_register_function_parent_class = g_type_class_peek_parent (klass);
	VALA_TYPEREGISTER_FUNCTION_CLASS (klass)->finalize = vala_enum_register_function_finalize;
	g_type_class_add_private (klass, sizeof (ValaEnumRegisterFunctionPrivate));
	VALA_TYPEREGISTER_FUNCTION_CLASS (klass)->get_type_declaration = vala_enum_register_function_real_get_type_declaration;
	VALA_TYPEREGISTER_FUNCTION_CLASS (klass)->get_accessibility = vala_enum_register_function_real_get_accessibility;
}


static void vala_enum_register_function_instance_init (ValaEnumRegisterFunction * self) {
	self->priv = VALA_ENUM_REGISTER_FUNCTION_GET_PRIVATE (self);
}


static void vala_enum_register_function_finalize (ValaTypeRegisterFunction* obj) {
	ValaEnumRegisterFunction * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, VALA_TYPE_ENUM_REGISTER_FUNCTION, ValaEnumRegisterFunction);
	VALA_TYPEREGISTER_FUNCTION_CLASS (vala_enum_register_function_parent_class)->finalize (obj);
}


/**
 * C function to register an enum at runtime.
 */
GType vala_enum_register_function_get_type (void) {
	static volatile gsize vala_enum_register_function_type_id__volatile = 0;
	if (g_once_init_enter (&vala_enum_register_function_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (ValaEnumRegisterFunctionClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) vala_enum_register_function_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (ValaEnumRegisterFunction), 0, (GInstanceInitFunc) vala_enum_register_function_instance_init, NULL };
		GType vala_enum_register_function_type_id;
		vala_enum_register_function_type_id = g_type_register_static (VALA_TYPE_TYPEREGISTER_FUNCTION, "ValaEnumRegisterFunction", &g_define_type_info, 0);
		g_once_init_leave (&vala_enum_register_function_type_id__volatile, vala_enum_register_function_type_id);
	}
	return vala_enum_register_function_type_id__volatile;
}



