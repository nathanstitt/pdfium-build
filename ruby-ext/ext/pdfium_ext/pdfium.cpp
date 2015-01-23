#include <stdlib.h>
#include <inttypes.h>
extern "C" {
#include "ruby.h"
}
#include <iostream>
#include <string>

#include "fpdf_dataavail.h"
#include "fpdf_ext.h"
#include "fpdfformfill.h"
#include "fpdftext.h"
#include "fpdfview.h"
//#include "v8/v8.h"

#include <assert.h>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "pdf.h"
#include "page.h"


// file local variables that are set in Init_pdfium_ext function
// and are referenced elsewhere in file
static ID to_s;        // used for calling the to_s method on file paths
static VALUE rb_pdf;   // Ruby definition of the Pdf class
static VALUE rb_page;  // Ruby definition of the Page class


/////////////////////////////////////////////////////////////////////////
// The Pdf class                                                       //
/////////////////////////////////////////////////////////////////////////

// a utility method to extract the reference to the Pdf class from the Ruby wrapping
static Pdf*
getPdf(VALUE self) {
  Pdf* p;
  Data_Get_Struct(self, Pdf, p);
  return p;
}

static void
pdf_gc_free(Pdf* pdf) {
    //    printf("GC Free PDF: %016" PRIxPTR "\n", (uintptr_t)pdf);
    // we need to call the destructor ourselves since we're using xmalloc and xfree
    pdf->~Pdf();
    ruby_xfree(pdf);
}

// static void
// pdf_gc_mark(Pdf *pdf){
//     // printf("GC Mark PDF: %016" PRIxPTR "\n", (uintptr_t)pdf);
// }


static VALUE
pdf_allocate(VALUE klass) {
    Pdf *pdf = (Pdf*)ruby_xmalloc(sizeof(Pdf));
    new (pdf) Pdf();
    return Data_Wrap_Struct(klass, NULL, pdf_gc_free, pdf );
}

static VALUE
pdf_initialize(VALUE self, VALUE _path) {
    const char* path = StringValuePtr(_path);
    Pdf* p = getPdf(self);
    if (! p->initialize(path) ){
        rb_raise(rb_eRuntimeError, "Unable to parse %s", path);
    }
    return Qnil;
}


static VALUE
pdf_page_count(VALUE klass){
    return INT2NUM( getPdf(klass)->pageCount() );
}



/////////////////////////////////////////////////////////////////////////
// The Page class                                                      //
/////////////////////////////////////////////////////////////////////////

// a utility method to extract the reference to the Page class from the Ruby wrapping
static Page*
getPage(VALUE self) {
  Page* p;
  Data_Get_Struct(self, Page, p);
  return p;
}

static void
page_gc_free(Page* page) {
    //    printf("GC Free PDF: %016" PRIxPTR "\n", (uintptr_t)pdf);
    // we need to call the destructor ourselves since we're using xmalloc and xfree
    page->~Page();
    ruby_xfree(page);
}


static VALUE
page_allocate(VALUE klass){
    Page *page = (Page*)ruby_xmalloc(sizeof(Page));
    new (page) Page();
    return Data_Wrap_Struct(klass, NULL, page_gc_free, page);
}

static VALUE
page_initialize(VALUE self, VALUE _pdf, VALUE page_number) {
    Page *pg = getPage(self);
    Pdf* pdf = getPdf(_pdf);

    if (!pg->initialize(pdf, FIX2INT(page_number))){
        rb_raise(rb_eRuntimeError, "Unable to load page %d", FIX2INT(page_number));
    }
    return Qnil;
}

static VALUE
page_dimensions(VALUE self){
    Page *pg = getPage(self);
    return rb_ary_new3(2, rb_float_new(pg->width()), rb_float_new(pg->height()) );
}

static VALUE
page_render(VALUE self, VALUE path, VALUE width, VALUE height){
    Page *pg = getPage(self);
    VALUE str = rb_funcall(path, to_s, 0);
    return pg->render( StringValuePtr(str), FIX2INT(width), FIX2INT(height) ) ? Qtrue : Qfalse;
}


extern "C"
void Init_pdfium_ext()
{
    //v8::V8::InitializeICU();

    Pdf::Initialize();

    to_s = rb_intern ("to_s");

    // Get symbol for PDFium
    ID sym_pdfium = rb_intern("PDFium");
    // Get the module
    VALUE rb_pdfium_module = rb_const_get(rb_cObject, sym_pdfium);


    // The Pdf class definition and methods
    rb_pdf = rb_define_class_under(rb_pdfium_module,"Pdf",  rb_cObject);
    rb_define_alloc_func(rb_pdf, pdf_allocate);
    rb_define_private_method(rb_pdf, "initialize",
                             reinterpret_cast<VALUE(*)(...)>(pdf_initialize), 1);

    rb_define_method(rb_pdf, "page_count",
                     reinterpret_cast<VALUE(*)(...)>(pdf_page_count),0);

    rb_define_method(rb_pdf, "page_at",
                     reinterpret_cast<VALUE(*)(...)>(pdf_page_count),1);

    // The Page class definition and methods
    rb_page = rb_define_class_under(rb_pdfium_module, "Page", rb_cObject);

    rb_define_alloc_func(rb_page, page_allocate);
    rb_define_private_method(rb_page, "initialize",
                             reinterpret_cast<VALUE(*)(...)>(page_initialize), 2);

    rb_define_method(rb_page, "dimensions",
                     reinterpret_cast<VALUE(*)(...)>(page_dimensions),0);

    rb_define_method(rb_page, "render",
                     reinterpret_cast<VALUE(*)(...)>(page_render),3);



}
