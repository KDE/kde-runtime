/*
 * preproc.c: Preprocessing of style operations
 *
 * References:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 *   Michael Kay "XSLT Programmer's Reference" pp 637-643
 *   Writing Multiple Output Files
 *
 *   XSLT-1.1 Working Draft
 *   http://www.w3.org/TR/xslt11#multiple-output
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include "xsltconfig.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/uri.h>
#include <libxml/xmlerror.h>
#include "xslt.h"
#include "xsltutils.h"
#include "xsltInternals.h"
#include "transform.h"
#include "templates.h"
#include "variables.h"
#include "numbersInternals.h"
#include "preproc.h"
#include "extra.h"
#include "imports.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_PREPROC
#endif

const xmlChar *xsltExtMarker = (const xmlChar *) "Extension Element";

/************************************************************************
 *									*
 *			handling of precomputed data			*
 *									*
 ************************************************************************/

/**
 * xsltNewStylePreComp:
 * @style:  the XSLT stylesheet
 * @type:  the construct type
 *
 * Create a new XSLT Style precomputed block
 *
 * Returns the newly allocated xsltStylePreCompPtr or NULL in case of error
 */
static xsltStylePreCompPtr
xsltNewStylePreComp(xsltStylesheetPtr style, xsltStyleType type) {
    xsltStylePreCompPtr cur;

    cur = (xsltStylePreCompPtr) xmlMalloc(sizeof(xsltStylePreComp));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewStylePreComp : malloc failed\n");
	style->errors++;
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltStylePreComp));

    cur->type = type;
    switch (cur->type) {
        case XSLT_FUNC_COPY:
            cur->func = xsltCopy;break;
        case XSLT_FUNC_SORT:
            cur->func = xsltSort;break;
        case XSLT_FUNC_TEXT:
            cur->func = xsltText;break;
        case XSLT_FUNC_ELEMENT:
            cur->func = xsltElement;break;
        case XSLT_FUNC_ATTRIBUTE:
            cur->func = xsltAttribute;break;
        case XSLT_FUNC_COMMENT:
            cur->func = xsltComment;break;
        case XSLT_FUNC_PI:
            cur->func = xsltProcessingInstruction;break;
        case XSLT_FUNC_COPYOF:
            cur->func = xsltCopyOf;break;
        case XSLT_FUNC_VALUEOF:
            cur->func = xsltValueOf;break;
        case XSLT_FUNC_NUMBER:
            cur->func = xsltNumber;break;
        case XSLT_FUNC_APPLYIMPORTS:
            cur->func = xsltApplyImports;break;
        case XSLT_FUNC_CALLTEMPLATE:
            cur->func = xsltCallTemplate;break;
        case XSLT_FUNC_APPLYTEMPLATES:
            cur->func = xsltApplyTemplates;break;
        case XSLT_FUNC_CHOOSE:
            cur->func = xsltChoose;break;
        case XSLT_FUNC_IF:
            cur->func = xsltIf;break;
        case XSLT_FUNC_FOREACH:
            cur->func = xsltForEach;break;
        case XSLT_FUNC_DOCUMENT:
            cur->func = xsltDocumentElem;break;
	case XSLT_FUNC_WITHPARAM:
	    cur->func = NULL;break;
	case XSLT_FUNC_PARAM:
	    cur->func = NULL;break;
	case XSLT_FUNC_VARIABLE:
	    cur->func = NULL;break;
	case XSLT_FUNC_WHEN:
	    cur->func = NULL;break;
	default:
	if (cur->func == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltNewStylePreComp : no function for type %d\n", type);
	    style->errors++;
	}
    }
    cur->next = style->preComps;
    style->preComps = cur;

    return(cur);
}

/**
 * xsltFreeStylePreComp:
 * @comp:  an XSLT Style precomputed block
 *
 * Free up the memory allocated by @comp
 */
static void
xsltFreeStylePreComp(xsltStylePreCompPtr comp) {
    if (comp == NULL)
	return;
    if (comp->stype != NULL)
	xmlFree(comp->stype);
    if (comp->order != NULL)
	xmlFree(comp->order);
    if (comp->use != NULL)
	xmlFree(comp->use);
    if (comp->name != NULL)
	xmlFree(comp->name);
    if (comp->ns != NULL)
	xmlFree(comp->ns);
    if (comp->mode != NULL)
	xmlFree(comp->mode);
    if (comp->modeURI != NULL)
	xmlFree(comp->modeURI);
    if (comp->test != NULL)
	xmlFree(comp->test);
    if (comp->select != NULL)
	xmlFree(comp->select);

    if (comp->filename != NULL)
	xmlFree(comp->filename);

    if (comp->numdata.level != NULL)
	xmlFree(comp->numdata.level);
    if (comp->numdata.count != NULL)
	xmlFree(comp->numdata.count);
    if (comp->numdata.from != NULL)
	xmlFree(comp->numdata.from);
    if (comp->numdata.value != NULL)
	xmlFree(comp->numdata.value);
    if (comp->numdata.format != NULL)
	xmlFree(comp->numdata.format);
    if (comp->comp != NULL)
	xmlXPathFreeCompExpr(comp->comp);
    if (comp->nsList != NULL)
	xmlFree(comp->nsList);

    xmlFree(comp);
}


/************************************************************************
 *									*
 *		    XSLT-1.1 extensions					*
 *									*
 ************************************************************************/

/**
 * xsltDocumentComp:
 * @style:  the XSLT stylesheet
 * @inst:  the instruction in the stylesheet
 *
 * Pre process an XSLT-1.1 document element
 */
static void
xsltDocumentComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *filename = NULL;
    xmlChar *base = NULL;
    xmlChar *URL = NULL;

    comp = xsltNewStylePreComp(style, XSLT_FUNC_DOCUMENT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
    comp->ver11 = 0;

    if (xmlStrEqual(inst->name, (const xmlChar *) "output")) {
#ifdef WITH_XSLT_DEBUG_EXTRA
	xsltGenericDebug(xsltGenericDebugContext,
	    "Found saxon:output extension\n");
#endif
	filename = xsltEvalStaticAttrValueTemplate(style, inst,
			 (const xmlChar *)"file",
			 XSLT_SAXON_NAMESPACE, &comp->has_filename);
    } else if (xmlStrEqual(inst->name, (const xmlChar *) "write")) {
#ifdef WITH_XSLT_DEBUG_EXTRA
	xsltGenericDebug(xsltGenericDebugContext,
	    "Found xalan:write extension\n");
#endif
	filename = xsltEvalStaticAttrValueTemplate(style, inst,
			 (const xmlChar *)"select",
			 XSLT_XALAN_NAMESPACE, &comp->has_filename);
    } else if (xmlStrEqual(inst->name, (const xmlChar *) "document")) {
	filename = xsltEvalStaticAttrValueTemplate(style, inst,
			 (const xmlChar *)"href",
			 XSLT_XT_NAMESPACE, &comp->has_filename);
	if (comp->has_filename == 0) {
#ifdef WITH_XSLT_DEBUG_EXTRA
	    xsltGenericDebug(xsltGenericDebugContext,
		"Found xslt11:document construct\n");
#endif
	    filename = xsltEvalStaticAttrValueTemplate(style, inst,
			     (const xmlChar *)"href",
			     XSLT_NAMESPACE, &comp->has_filename);
	    comp->ver11 = 1;
	} else {
#ifdef WITH_XSLT_DEBUG_EXTRA
	    xsltGenericDebug(xsltGenericDebugContext,
		"Found xt:document extension\n");
#endif
	    comp->ver11 = 0;
	}
    }
    if (!comp->has_filename) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltDocumentComp: could not find the href\n");
	style->errors++;
	goto error;
    }

    if (filename != NULL) {
	/*
	 * Compute output URL
	 */
	base = xmlNodeGetBase(inst->doc, inst);
	URL = xmlBuildURI(filename, base);
	if (URL == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltDocumentComp: URL computation failed %s\n", filename);
	    style->warnings++;
	    comp->filename = xmlStrdup(filename);
	} else {
	    comp->filename = URL;
	}
    } else {
	comp->filename = NULL;
    }

error:
    if (base != NULL)
	xmlFree(base);
    if (filename != NULL)
	xmlFree(filename);
}

/************************************************************************
 *									*
 *		Most of the XSLT-1.0 transformations			*
 *									*
 ************************************************************************/

/**
 * xsltSortComp:
 * @style:  the XSLT stylesheet
 * @inst:  the xslt sort node
 *
 * Process the xslt sort node on the source node
 */
static void
xsltSortComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;


    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_SORT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->stype = xsltEvalStaticAttrValueTemplate(style, inst,
			 (const xmlChar *)"data-type",
			 XSLT_NAMESPACE, &comp->has_stype);
    if (comp->stype != NULL) {
	if (xmlStrEqual(comp->stype, (const xmlChar *) "text"))
	    comp->number = 0;
	else if (xmlStrEqual(comp->stype, (const xmlChar *) "number"))
	    comp->number = 1;
	else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltSortComp: no support for data-type = %s\n", comp->stype);
	    comp->number = -1;
	    style->warnings++;
	}
    }
    comp->order = xsltEvalStaticAttrValueTemplate(style, inst,
			      (const xmlChar *)"order",
			      XSLT_NAMESPACE, &comp->has_order);
    if (comp->order != NULL) {
	if (xmlStrEqual(comp->order, (const xmlChar *) "ascending"))
	    comp->descending = 0;
	else if (xmlStrEqual(comp->order, (const xmlChar *) "descending"))
	    comp->descending = 1;
	else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltSortComp: invalid value %s for order\n", comp->order);
	    comp->descending = -1;
	    style->warnings++;
	}
    }
    /* TODO: xsl:sort lang attribute */
    /* TODO: xsl:sort case-order attribute */

    comp->select = xmlGetNsProp(inst,(const xmlChar *)"select", XSLT_NAMESPACE);
    if (comp->select == NULL) {
	/*
	 * The default value of the select attribute is ., which will
	 * cause the string-value of the current node to be used as
	 * the sort key.
	 */
	comp->select = xmlStrdup((const xmlChar *)".");
    }
    comp->comp = xmlXPathCompile(comp->select);
    if (comp->comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsltSortComp: could not compile select expression '%s'\n",
	                 comp->select);
	style->errors++;
    }
}

/**
 * xsltCopyComp:
 * @style:  the XSLT stylesheet
 * @inst:  the xslt copy node
 *
 * Process the xslt copy node on the source node
 */
static void
xsltCopyComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;


    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_COPY);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;


    comp->use = xmlGetNsProp(inst, (const xmlChar *)"use-attribute-sets",
				    XSLT_NAMESPACE);
    if (comp->use == NULL)
	comp->has_use = 0;
    else
	comp->has_use = 1;
}

/**
 * xsltTextComp:
 * @style:  a XSLT process context
 * @inst:  the xslt text node
 *
 * Process the xslt text node on the source node
 */
static void
xsltTextComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_TEXT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
    comp->noescape = 0;

    prop = xmlGetNsProp(inst,
	    (const xmlChar *)"disable-output-escaping",
			XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    comp->noescape = 1;
	} else if (!xmlStrEqual(prop,
				(const xmlChar *)"no")){
	    xsltGenericError(xsltGenericErrorContext,
"xslt:text: disable-output-escaping allow only yes or no\n");
	    style->warnings++;
	}
	xmlFree(prop);
    }
}

/**
 * xsltElementComp:
 * @style:  a XSLT process context
 * @inst:  the xslt element node
 *
 * Process the xslt element node on the source node
 */
static void
xsltElementComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_ELEMENT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * TODO: more computation can be done there, especially namespace lookup
     */
    comp->name = xsltEvalStaticAttrValueTemplate(style, inst,
				 (const xmlChar *)"name",
				 XSLT_NAMESPACE, &comp->has_name);
    comp->ns = xsltEvalStaticAttrValueTemplate(style, inst,
			 (const xmlChar *)"namespace",
			 XSLT_NAMESPACE, &comp->has_ns);

    comp->use = xsltEvalStaticAttrValueTemplate(style, inst,
		       (const xmlChar *)"use-attribute-sets",
		       XSLT_NAMESPACE, &comp->has_use);
}

/**
 * xsltAttributeComp:
 * @style:  a XSLT process context
 * @inst:  the xslt attribute node
 *
 * Process the xslt attribute node on the source node
 */
static void
xsltAttributeComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_ATTRIBUTE);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * TODO: more computation can be done there, especially namespace lookup
     */
    comp->name = xsltEvalStaticAttrValueTemplate(style, inst,
				 (const xmlChar *)"name",
				 XSLT_NAMESPACE, &comp->has_name);
    comp->ns = xsltEvalStaticAttrValueTemplate(style, inst,
			 (const xmlChar *)"namespace",
			 XSLT_NAMESPACE, &comp->has_ns);

}

/**
 * xsltCommentComp:
 * @style:  a XSLT process context
 * @inst:  the xslt comment node
 *
 * Process the xslt comment node on the source node
 */
static void
xsltCommentComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_COMMENT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
}

/**
 * xsltProcessingInstructionComp:
 * @style:  a XSLT process context
 * @inst:  the xslt processing-instruction node
 *
 * Process the xslt processing-instruction node on the source node
 */
static void
xsltProcessingInstructionComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_PI);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->name = xsltEvalStaticAttrValueTemplate(style, inst,
				 (const xmlChar *)"name",
				 XSLT_NAMESPACE, &comp->has_name);
}

/**
 * xsltCopyOfComp:
 * @style:  a XSLT process context
 * @inst:  the xslt copy-of node
 *
 * Process the xslt copy-of node on the source node
 */
static void
xsltCopyOfComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_COPYOF);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:copy-of : select is missing\n");
	style->errors++;
	return;
    }
    comp->comp = xmlXPathCompile(comp->select);
    if (comp->comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:copy-of : could not compile select expression '%s'\n",
	                 comp->select);
	style->errors++;
    }
}

/**
 * xsltValueOfComp:
 * @style:  a XSLT process context
 * @inst:  the xslt value-of node
 *
 * Process the xslt value-of node on the source node
 */
static void
xsltValueOfComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_VALUEOF);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    prop = xmlGetNsProp(inst,
	    (const xmlChar *)"disable-output-escaping",
			XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    comp->noescape = 1;
	} else if (!xmlStrEqual(prop,
				(const xmlChar *)"no")){
	    xsltGenericError(xsltGenericErrorContext,
"value-of: disable-output-escaping allow only yes or no\n");
	    style->warnings++;
	}
	xmlFree(prop);
    }
    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:value-of : select is missing\n");
	style->errors++;
	return;
    }
    comp->comp = xmlXPathCompile(comp->select);
    if (comp->comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:value-of : could not compile select expression '%s'\n",
	                 comp->select);
	style->errors++;
    }
}

/**
 * xsltWithParamComp:
 * @style:  a XSLT process context
 * @inst:  the xslt with-param node
 *
 * Process the xslt with-param node on the source node
 */
static void
xsltWithParamComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlNsPtr ns = NULL;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_WITHPARAM);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * The full namespace resolution can be done statically
     */
    prop = xmlGetNsProp(inst, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:with-param : name is missing\n");
	style->errors++;
    } else {

	ncname = xmlSplitQName2(prop, &prefix);
	if (ncname == NULL) {
	    ncname = prop;
	    prop = NULL;
	    prefix = NULL;
	}
	if (prefix != NULL) {
	    ns = xmlSearchNs(inst->doc, inst, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		"xslt:with-param : no namespace bound to prefix %s\n", prefix);
		style->warnings++;
	    }
	}
	comp->name = xmlStrdup(ncname);
	comp->has_name = 1;
	if (ns != NULL) {
	    comp->has_ns = 1;
	    comp->ns = xmlStrdup(ns->href);
	} else
	    comp->has_ns = 0;
    }

    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select != NULL) {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:param : could not compile select expression '%s'\n",
			     comp->select);
	    style->errors++;
	}
	if (inst->children != NULL)
	    xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : content should be empty since select is present \n");
	style->warnings++;
    }

    if (prop != NULL)
        xmlFree(prop);
    if (ncname != NULL)
        xmlFree(ncname);
    if (prefix != NULL)
        xmlFree(prefix);
}

/**
 * xsltNumberComp:
 * @style:  a XSLT process context
 * @cur:   the xslt number node
 *
 * Process the xslt number node on the source node
 */
static void
xsltNumberComp(xsltStylesheetPtr style, xmlNodePtr cur) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;

    if ((style == NULL) || (cur == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_NUMBER);
    if (comp == NULL)
	return;
    cur->_private = comp;

    if ((style == NULL) || (cur == NULL))
	return;

    comp->numdata.doc = cur->doc;
    comp->numdata.node = cur;
    comp->numdata.value = xmlGetNsProp(cur, (const xmlChar *)"value",
	                                XSLT_NAMESPACE);
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"format", XSLT_NAMESPACE);
    if (prop != NULL) {
	comp->numdata.format = prop;
    } else {
	comp->numdata.format = xmlStrdup(BAD_CAST(""));
    }
    
    comp->numdata.count = xmlGetNsProp(cur, (const xmlChar *)"count",
					XSLT_NAMESPACE);
    comp->numdata.from = xmlGetNsProp(cur, (const xmlChar *)"from",
					XSLT_NAMESPACE);
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"level", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, BAD_CAST("single")) ||
	    xmlStrEqual(prop, BAD_CAST("multiple")) ||
	    xmlStrEqual(prop, BAD_CAST("any"))) {
	    comp->numdata.level = prop;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
			 "xsl:number : invalid value %s for level\n", prop);
	    style->warnings++;
	    xmlFree(prop);
	}
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"lang", XSLT_NAMESPACE);
    if (prop != NULL) {
	TODO; /* xsl:number lang attribute */
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"letter-value", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, BAD_CAST("alphabetic"))) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsl:number : letter-value 'alphabetic' not implemented\n");
	    style->warnings++;
	    TODO; /* xsl:number letter-value attribute alphabetic */
	} else if (xmlStrEqual(prop, BAD_CAST("traditional"))) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsl:number : letter-value 'traditional' not implemented\n");
	    style->warnings++;
	    TODO; /* xsl:number letter-value attribute traditional */
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		     "xsl:number : invalid value %s for letter-value\n", prop);
	    style->warnings++;
	}
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"grouping-separator",
	                XSLT_NAMESPACE);
    if (prop != NULL) {
	comp->numdata.groupingCharacter = prop[0];
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"grouping-size", XSLT_NAMESPACE);
    if (prop != NULL) {
	sscanf((char *)prop, "%d", &comp->numdata.digitsPerGroup);
	xmlFree(prop);
    } else {
	comp->numdata.groupingCharacter = 0;
    }

    /* Set default values */
    if (comp->numdata.value == NULL) {
	if (comp->numdata.level == NULL) {
	    comp->numdata.level = xmlStrdup(BAD_CAST("single"));
	}
    }
    
}

/**
 * xsltApplyImportsComp:
 * @style:  a XSLT process context
 * @inst:  the xslt apply-imports node
 *
 * Process the xslt apply-imports node on the source node
 */
static void
xsltApplyImportsComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_APPLYIMPORTS);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
}

/**
 * xsltCallTemplateComp:
 * @style:  a XSLT process context
 * @inst:  the xslt call-template node
 *
 * Process the xslt call-template node on the source node
 */
static void
xsltCallTemplateComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlNsPtr ns = NULL;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_CALLTEMPLATE);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * The full template resolution can be done statically
     */
    prop = xmlGetNsProp(inst, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:call-template : name is missing\n");
	style->errors++;
    } else {

	ncname = xmlSplitQName2(prop, &prefix);
	if (ncname == NULL) {
	    ncname = prop;
	    prop = NULL;
	    prefix = NULL;
	}
	if (prefix != NULL) {
	    ns = xmlSearchNs(inst->doc, inst, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
	"xslt:call-template : no namespace bound to prefix %s\n", prefix);
		style->warnings++;
	    }
	}
	comp->name = xmlStrdup(ncname);
	comp->has_name = 1;
	if (ns != NULL) {
	    comp->ns = xmlStrdup(ns->href);
	    comp->has_ns = 1;
	} else
	    comp->has_ns = 0;
	comp->templ = NULL;
    }

    if (prop != NULL)
        xmlFree(prop);
    if (ncname != NULL)
        xmlFree(ncname);
    if (prefix != NULL)
        xmlFree(prefix);
}

/**
 * xsltApplyTemplatesComp:
 * @style:  a XSLT process context
 * @inst:  the apply-templates node
 *
 * Process the apply-templates node on the source node
 */
static void
xsltApplyTemplatesComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_APPLYTEMPLATES);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * Get mode if any
     */
    prop = xmlGetNsProp(inst, (const xmlChar *)"mode", XSLT_NAMESPACE);
    if (prop != NULL) {
	xmlChar *prefix = NULL;

	comp->mode = xmlSplitQName2(prop, &prefix);
	if (comp->mode != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(inst->doc, inst, prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"no namespace bound to prefix %s\n", prefix);
		    style->warnings++;
		    xmlFree(prefix);
		    xmlFree(comp->mode);
		    comp->mode = prop;
		    comp->modeURI = NULL;
		} else {
		    comp->modeURI = xmlStrdup(ns->href);
		    xmlFree(prefix);
		    xmlFree(prop);
		}
	    } else {
		xmlFree(prop);
		comp->modeURI = NULL;
	    }
	} else {
	    comp->mode = prop;
	    comp->modeURI = NULL;
	}
    }
    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select != NULL) {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
     "xslt:apply-templates : could not compile select expression '%s'\n",
			     comp->select);
	    style->errors++;
	}
    }

    /* TODO: handle (or skip) the xsl:sort and xsl:with-param */
}

/**
 * xsltChooseComp:
 * @style:  a XSLT process context
 * @inst:  the xslt choose node
 *
 * Process the xslt choose node on the source node
 */
static void
xsltChooseComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_CHOOSE);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
}

/**
 * xsltIfComp:
 * @style:  a XSLT process context
 * @inst:  the xslt if node
 *
 * Process the xslt if node on the source node
 */
static void
xsltIfComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_IF);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->test = xmlGetNsProp(inst, (const xmlChar *)"test", XSLT_NAMESPACE);
    if (comp->test == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:if : test is not defined\n");
	style->errors++;
	return;
    }
    comp->comp = xmlXPathCompile(comp->test);
    if (comp->comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:if : could not compile test expression '%s'\n",
	                 comp->test);
	style->errors++;
    }
}

/**
 * xsltWhenComp:
 * @style:  a XSLT process context
 * @inst:  the xslt if node
 *
 * Process the xslt if node on the source node
 */
static void
xsltWhenComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_WHEN);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->test = xmlGetNsProp(inst, (const xmlChar *)"test", XSLT_NAMESPACE);
    if (comp->test == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:when : test is not defined\n");
	style->errors++;
	return;
    }
    comp->comp = xmlXPathCompile(comp->test);
    if (comp->comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:when : could not compile test expression '%s'\n",
	                 comp->test);
	style->errors++;
    }
}

/**
 * xsltForEachComp:
 * @style:  a XSLT process context
 * @inst:  the xslt for-each node
 *
 * Process the xslt for-each node on the source node
 */
static void
xsltForEachComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_FOREACH);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select == NULL) {
	xsltGenericError(xsltGenericErrorContext,
		"xslt:for-each : select is missing\n");
	style->errors++;
    } else {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
     "xslt:for-each : could not compile select expression '%s'\n",
			     comp->select);
	    style->errors++;
	}
    }
    /* TODO: handle and skip the xsl:sort */
}

/**
 * xsltVariableComp:
 * @style:  a XSLT process context
 * @inst:  the xslt variable node
 *
 * Process the xslt variable node on the source node
 */
static void
xsltVariableComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlNsPtr ns = NULL;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_VARIABLE);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * The full template resolution can be done statically
     */
    prop = xmlGetNsProp(inst, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:variable : name is missing\n");
	style->errors++;
    } else {
	ncname = xmlSplitQName2(prop, &prefix);
	if (ncname == NULL) {
	    ncname = prop;
	    prop = NULL;
	    prefix = NULL;
	}
	if (prefix != NULL) {
	    ns = xmlSearchNs(inst->doc, inst, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		"xsl:variable no namespace bound to prefix %s\n", prefix);
		style->warnings++;
	    }
	}
	comp->name = xmlStrdup(ncname);
	comp->has_name = 1;
	if (ns != NULL) {
	    comp->ns = xmlStrdup(ns->href);
	    comp->has_ns = 1;
	} else
	    comp->has_ns = 0;
    }

    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select != NULL) {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:variable : could not compile select expression '%s'\n",
			     comp->select);
	    style->errors++;
	}
	if (inst->children != NULL)
	    xsltGenericError(xsltGenericErrorContext,
	"xsl:variable : content should be empty since select is present \n");
	style->warnings++;
    }

    if (prop != NULL)
        xmlFree(prop);
    if (ncname != NULL)
        xmlFree(ncname);
    if (prefix != NULL)
        xmlFree(prefix);
}

/**
 * xsltParamComp:
 * @style:  a XSLT process context
 * @inst:  the xslt param node
 *
 * Process the xslt param node on the source node
 */
static void
xsltParamComp(xsltStylesheetPtr style, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlNsPtr ns = NULL;

    if ((style == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(style, XSLT_FUNC_PARAM);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * The full template resolution can be done statically
     */
    prop = xmlGetNsProp(inst, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:param : name is missing\n");
	style->errors++;
    } else {
	ncname = xmlSplitQName2(prop, &prefix);
	if (ncname == NULL) {
	    ncname = prop;
	    prop = NULL;
	    prefix = NULL;
	}
	if (prefix != NULL) {
	    ns = xmlSearchNs(inst->doc, inst, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "xsl:param no namespace bound to prefix %s\n", prefix);
		style->warnings++;
	    }
	}
	comp->name = xmlStrdup(ncname);
	comp->has_name = 1;
	if (ns != NULL) {
	    comp->ns = xmlStrdup(ns->href);
	    comp->has_ns = 1;
	} else
	    comp->has_ns = 0;
    }

    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select != NULL) {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:param : could not compile select expression '%s'\n",
			     comp->select);
	    style->errors++;
	}
	if (inst->children != NULL)
	    xsltGenericError(xsltGenericErrorContext,
	"xsl:param : content should be empty since select is present \n");
	style->warnings++;
    }

    if (prop != NULL)
        xmlFree(prop);
    if (ncname != NULL)
        xmlFree(ncname);
    if (prefix != NULL)
        xmlFree(prefix);
}


/************************************************************************
 *									*
 *		    Generic interface					*
 *									*
 ************************************************************************/

/**
 * xsltFreeStylePreComps:
 * @style:  an XSLT transformation context
 *
 * Free up the memory allocated by all precomputed blocks
 */
void
xsltFreeStylePreComps(xsltStylesheetPtr style) {
    xsltStylePreCompPtr cur, next;

    if (style == NULL)
	return;
    cur = style->preComps;
    while (cur != NULL) {
	next = cur->next;
	xsltFreeStylePreComp(cur);
	cur = next;
    }
}

/**
 * xsltStylePreCompute:
 * @style:  the XSLT stylesheet
 * @inst:  the instruction in the stylesheet
 *
 * Precompute an XSLT stylesheet element
 */
void
xsltStylePreCompute(xsltStylesheetPtr style, xmlNodePtr inst) {
    if (inst->_private != NULL) 
        return;
    if (IS_XSLT_ELEM(inst)) {
	xsltStylePreCompPtr cur;

	if (IS_XSLT_NAME(inst, "apply-templates")) {
	    xsltApplyTemplatesComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "with-param")) {
	    xsltWithParamComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "value-of")) {
	    xsltValueOfComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "copy")) {
	    xsltCopyComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "copy-of")) {
	    xsltCopyOfComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "if")) {
	    xsltIfComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "when")) {
	    xsltWhenComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "choose")) {
	    xsltChooseComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "for-each")) {
	    xsltForEachComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "apply-imports")) {
	    xsltApplyImportsComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "attribute")) {
	    xsltAttributeComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "element")) {
	    xsltElementComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "text")) {
	    xsltTextComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "sort")) {
	    xsltSortComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "comment")) {
	    xsltCommentComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "number")) {
	    xsltNumberComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "processing-instruction")) {
	    xsltProcessingInstructionComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "call-template")) {
	    xsltCallTemplateComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "param")) {
	    xsltParamComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "variable")) {
	    xsltVariableComp(style, inst);
	} else if (IS_XSLT_NAME(inst, "otherwise")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "template")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "output")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "preserve-space")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "strip-space")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "stylesheet")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "key")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "message")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "attribute-set")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "namespace-alias")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "include")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "import")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "decimal-format")) {
	    /* no computation needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "document")) {
	    xsltDocumentComp(style, inst);
	} else {
	    xsltGenericError(xsltGenericDebugContext,
		 "xsltStylePreCompute: unknown xslt:%s\n", inst->name);
	    style->warnings++;
	}
	/*
	 * Add the namespace lookup here, this code can be shared by
	 * all precomputations.
	 */
	cur = inst->_private;
	if (cur != NULL) {
	    int i = 0;

	    cur->nsList = xmlGetNsList(inst->doc, inst);
            if (cur->nsList != NULL) {
		while (cur->nsList[i] != NULL)
		    i++;
	    }
	    cur->nsNr = i;
	}
    } else {
	if (IS_XSLT_NAME(inst, "document")) {
	    xsltDocumentComp(style, inst);
	} else {
	    /*
	     * Mark the element for later recognition.
	     */
	    if (inst->_private == NULL)
		inst->_private = (void *) xsltExtMarker;
	}
    }
}
