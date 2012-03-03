// The MIT License
// 
// Copyright (c) 2011 Constellation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

(function(exp) {
  var IDENT = new Object(), EOS = 0, ILLEGAL = 1, NOTFOUND = 2,
      EXP = "EXP", STMT = "STMT", DECL = "DECL",
      GETTER = 1, SETTER = 2,
      IdentifyReservedWords = 1, IgnoreReservedWords = 2,
      IgnoreReservedWordsAndIdentifyGetterOrSetter = 3;


  var IdentifierStart = /^(?:[\u0024\u0041-\u005A\u005C\u005F\u0061-\u007A\u00AA\u00B5\u00BA\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u02C1\u02C6-\u02D1\u02E0-\u02E4\u02EC\u02EE\u0370-\u0374\u0376-\u0377\u037A-\u037D\u0386\u0388-\u038A\u038C\u038E-\u03A1\u03A3-\u03F5\u03F7-\u0481\u048A-\u0527\u0531-\u0556\u0559\u0561-\u0587\u05D0-\u05EA\u05F0-\u05F2\u0620-\u064A\u066E-\u066F\u0671-\u06D3\u06D5\u06E5-\u06E6\u06EE-\u06EF\u06FA-\u06FC\u06FF\u0710\u0712-\u072F\u074D-\u07A5\u07B1\u07CA-\u07EA\u07F4-\u07F5\u07FA\u0800-\u0815\u081A\u0824\u0828\u0840-\u0858\u08A0\u08A2-\u08AC\u0904-\u0939\u093D\u0950\u0958-\u0961\u0971-\u0977\u0979-\u097F\u0985-\u098C\u098F-\u0990\u0993-\u09A8\u09AA-\u09B0\u09B2\u09B6-\u09B9\u09BD\u09CE\u09DC-\u09DD\u09DF-\u09E1\u09F0-\u09F1\u0A05-\u0A0A\u0A0F-\u0A10\u0A13-\u0A28\u0A2A-\u0A30\u0A32-\u0A33\u0A35-\u0A36\u0A38-\u0A39\u0A59-\u0A5C\u0A5E\u0A72-\u0A74\u0A85-\u0A8D\u0A8F-\u0A91\u0A93-\u0AA8\u0AAA-\u0AB0\u0AB2-\u0AB3\u0AB5-\u0AB9\u0ABD\u0AD0\u0AE0-\u0AE1\u0B05-\u0B0C\u0B0F-\u0B10\u0B13-\u0B28\u0B2A-\u0B30\u0B32-\u0B33\u0B35-\u0B39\u0B3D\u0B5C-\u0B5D\u0B5F-\u0B61\u0B71\u0B83\u0B85-\u0B8A\u0B8E-\u0B90\u0B92-\u0B95\u0B99-\u0B9A\u0B9C\u0B9E-\u0B9F\u0BA3-\u0BA4\u0BA8-\u0BAA\u0BAE-\u0BB9\u0BD0\u0C05-\u0C0C\u0C0E-\u0C10\u0C12-\u0C28\u0C2A-\u0C33\u0C35-\u0C39\u0C3D\u0C58-\u0C59\u0C60-\u0C61\u0C85-\u0C8C\u0C8E-\u0C90\u0C92-\u0CA8\u0CAA-\u0CB3\u0CB5-\u0CB9\u0CBD\u0CDE\u0CE0-\u0CE1\u0CF1-\u0CF2\u0D05-\u0D0C\u0D0E-\u0D10\u0D12-\u0D3A\u0D3D\u0D4E\u0D60-\u0D61\u0D7A-\u0D7F\u0D85-\u0D96\u0D9A-\u0DB1\u0DB3-\u0DBB\u0DBD\u0DC0-\u0DC6\u0E01-\u0E30\u0E32-\u0E33\u0E40-\u0E46\u0E81-\u0E82\u0E84\u0E87-\u0E88\u0E8A\u0E8D\u0E94-\u0E97\u0E99-\u0E9F\u0EA1-\u0EA3\u0EA5\u0EA7\u0EAA-\u0EAB\u0EAD-\u0EB0\u0EB2-\u0EB3\u0EBD\u0EC0-\u0EC4\u0EC6\u0EDC-\u0EDF\u0F00\u0F40-\u0F47\u0F49-\u0F6C\u0F88-\u0F8C\u1000-\u102A\u103F\u1050-\u1055\u105A-\u105D\u1061\u1065-\u1066\u106E-\u1070\u1075-\u1081\u108E\u10A0-\u10C5\u10C7\u10CD\u10D0-\u10FA\u10FC-\u1248\u124A-\u124D\u1250-\u1256\u1258\u125A-\u125D\u1260-\u1288\u128A-\u128D\u1290-\u12B0\u12B2-\u12B5\u12B8-\u12BE\u12C0\u12C2-\u12C5\u12C8-\u12D6\u12D8-\u1310\u1312-\u1315\u1318-\u135A\u1380-\u138F\u13A0-\u13F4\u1401-\u166C\u166F-\u167F\u1681-\u169A\u16A0-\u16EA\u16EE-\u16F0\u1700-\u170C\u170E-\u1711\u1720-\u1731\u1740-\u1751\u1760-\u176C\u176E-\u1770\u1780-\u17B3\u17D7\u17DC\u1820-\u1877\u1880-\u18A8\u18AA\u18B0-\u18F5\u1900-\u191C\u1950-\u196D\u1970-\u1974\u1980-\u19AB\u19C1-\u19C7\u1A00-\u1A16\u1A20-\u1A54\u1AA7\u1B05-\u1B33\u1B45-\u1B4B\u1B83-\u1BA0\u1BAE-\u1BAF\u1BBA-\u1BE5\u1C00-\u1C23\u1C4D-\u1C4F\u1C5A-\u1C7D\u1CE9-\u1CEC\u1CEE-\u1CF1\u1CF5-\u1CF6\u1D00-\u1DBF\u1E00-\u1F15\u1F18-\u1F1D\u1F20-\u1F45\u1F48-\u1F4D\u1F50-\u1F57\u1F59\u1F5B\u1F5D\u1F5F-\u1F7D\u1F80-\u1FB4\u1FB6-\u1FBC\u1FBE\u1FC2-\u1FC4\u1FC6-\u1FCC\u1FD0-\u1FD3\u1FD6-\u1FDB\u1FE0-\u1FEC\u1FF2-\u1FF4\u1FF6-\u1FFC\u2071\u207F\u2090-\u209C\u2102\u2107\u210A-\u2113\u2115\u2119-\u211D\u2124\u2126\u2128\u212A-\u212D\u212F-\u2139\u213C-\u213F\u2145-\u2149\u214E\u2160-\u2188\u2C00-\u2C2E\u2C30-\u2C5E\u2C60-\u2CE4\u2CEB-\u2CEE\u2CF2-\u2CF3\u2D00-\u2D25\u2D27\u2D2D\u2D30-\u2D67\u2D6F\u2D80-\u2D96\u2DA0-\u2DA6\u2DA8-\u2DAE\u2DB0-\u2DB6\u2DB8-\u2DBE\u2DC0-\u2DC6\u2DC8-\u2DCE\u2DD0-\u2DD6\u2DD8-\u2DDE\u2E2F\u3005-\u3007\u3021-\u3029\u3031-\u3035\u3038-\u303C\u3041-\u3096\u309D-\u309F\u30A1-\u30FA\u30FC-\u30FF\u3105-\u312D\u3131-\u318E\u31A0-\u31BA\u31F0-\u31FF\u3400-\u4DB5\u4E00-\u9FCC\uA000-\uA48C\uA4D0-\uA4FD\uA500-\uA60C\uA610-\uA61F\uA62A-\uA62B\uA640-\uA66E\uA67F-\uA697\uA6A0-\uA6EF\uA717-\uA71F\uA722-\uA788\uA78B-\uA78E\uA790-\uA793\uA7A0-\uA7AA\uA7F8-\uA801\uA803-\uA805\uA807-\uA80A\uA80C-\uA822\uA840-\uA873\uA882-\uA8B3\uA8F2-\uA8F7\uA8FB\uA90A-\uA925\uA930-\uA946\uA960-\uA97C\uA984-\uA9B2\uA9CF\uAA00-\uAA28\uAA40-\uAA42\uAA44-\uAA4B\uAA60-\uAA76\uAA7A\uAA80-\uAAAF\uAAB1\uAAB5-\uAAB6\uAAB9-\uAABD\uAAC0\uAAC2\uAADB-\uAADD\uAAE0-\uAAEA\uAAF2-\uAAF4\uAB01-\uAB06\uAB09-\uAB0E\uAB11-\uAB16\uAB20-\uAB26\uAB28-\uAB2E\uABC0-\uABE2\uAC00-\uD7A3\uD7B0-\uD7C6\uD7CB-\uD7FB\uF900-\uFA6D\uFA70-\uFAD9\uFB00-\uFB06\uFB13-\uFB17\uFB1D\uFB1F-\uFB28\uFB2A-\uFB36\uFB38-\uFB3C\uFB3E\uFB40-\uFB41\uFB43-\uFB44\uFB46-\uFBB1\uFBD3-\uFD3D\uFD50-\uFD8F\uFD92-\uFDC7\uFDF0-\uFDFB\uFE70-\uFE74\uFE76-\uFEFC\uFF21-\uFF3A\uFF41-\uFF5A\uFF66-\uFFBE\uFFC2-\uFFC7\uFFCA-\uFFCF\uFFD2-\uFFD7\uFFDA-\uFFDC]|\u[da-fA-F]{4})/;
  var IdentifierPart = /^((?:[\u0024\u0030-\u0039\u0041-\u005A\u005C\u005F\u0061-\u007A\u00AA\u00B5\u00BA\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u02C1\u02C6-\u02D1\u02E0-\u02E4\u02EC\u02EE\u0300-\u0374\u0376-\u0377\u037A-\u037D\u0386\u0388-\u038A\u038C\u038E-\u03A1\u03A3-\u03F5\u03F7-\u0481\u0483-\u0487\u048A-\u0527\u0531-\u0556\u0559\u0561-\u0587\u0591-\u05BD\u05BF\u05C1-\u05C2\u05C4-\u05C5\u05C7\u05D0-\u05EA\u05F0-\u05F2\u0610-\u061A\u0620-\u0669\u066E-\u06D3\u06D5-\u06DC\u06DF-\u06E8\u06EA-\u06FC\u06FF\u0710-\u074A\u074D-\u07B1\u07C0-\u07F5\u07FA\u0800-\u082D\u0840-\u085B\u08A0\u08A2-\u08AC\u08E4-\u08FE\u0900-\u0963\u0966-\u096F\u0971-\u0977\u0979-\u097F\u0981-\u0983\u0985-\u098C\u098F-\u0990\u0993-\u09A8\u09AA-\u09B0\u09B2\u09B6-\u09B9\u09BC-\u09C4\u09C7-\u09C8\u09CB-\u09CE\u09D7\u09DC-\u09DD\u09DF-\u09E3\u09E6-\u09F1\u0A01-\u0A03\u0A05-\u0A0A\u0A0F-\u0A10\u0A13-\u0A28\u0A2A-\u0A30\u0A32-\u0A33\u0A35-\u0A36\u0A38-\u0A39\u0A3C\u0A3E-\u0A42\u0A47-\u0A48\u0A4B-\u0A4D\u0A51\u0A59-\u0A5C\u0A5E\u0A66-\u0A75\u0A81-\u0A83\u0A85-\u0A8D\u0A8F-\u0A91\u0A93-\u0AA8\u0AAA-\u0AB0\u0AB2-\u0AB3\u0AB5-\u0AB9\u0ABC-\u0AC5\u0AC7-\u0AC9\u0ACB-\u0ACD\u0AD0\u0AE0-\u0AE3\u0AE6-\u0AEF\u0B01-\u0B03\u0B05-\u0B0C\u0B0F-\u0B10\u0B13-\u0B28\u0B2A-\u0B30\u0B32-\u0B33\u0B35-\u0B39\u0B3C-\u0B44\u0B47-\u0B48\u0B4B-\u0B4D\u0B56-\u0B57\u0B5C-\u0B5D\u0B5F-\u0B63\u0B66-\u0B6F\u0B71\u0B82-\u0B83\u0B85-\u0B8A\u0B8E-\u0B90\u0B92-\u0B95\u0B99-\u0B9A\u0B9C\u0B9E-\u0B9F\u0BA3-\u0BA4\u0BA8-\u0BAA\u0BAE-\u0BB9\u0BBE-\u0BC2\u0BC6-\u0BC8\u0BCA-\u0BCD\u0BD0\u0BD7\u0BE6-\u0BEF\u0C01-\u0C03\u0C05-\u0C0C\u0C0E-\u0C10\u0C12-\u0C28\u0C2A-\u0C33\u0C35-\u0C39\u0C3D-\u0C44\u0C46-\u0C48\u0C4A-\u0C4D\u0C55-\u0C56\u0C58-\u0C59\u0C60-\u0C63\u0C66-\u0C6F\u0C82-\u0C83\u0C85-\u0C8C\u0C8E-\u0C90\u0C92-\u0CA8\u0CAA-\u0CB3\u0CB5-\u0CB9\u0CBC-\u0CC4\u0CC6-\u0CC8\u0CCA-\u0CCD\u0CD5-\u0CD6\u0CDE\u0CE0-\u0CE3\u0CE6-\u0CEF\u0CF1-\u0CF2\u0D02-\u0D03\u0D05-\u0D0C\u0D0E-\u0D10\u0D12-\u0D3A\u0D3D-\u0D44\u0D46-\u0D48\u0D4A-\u0D4E\u0D57\u0D60-\u0D63\u0D66-\u0D6F\u0D7A-\u0D7F\u0D82-\u0D83\u0D85-\u0D96\u0D9A-\u0DB1\u0DB3-\u0DBB\u0DBD\u0DC0-\u0DC6\u0DCA\u0DCF-\u0DD4\u0DD6\u0DD8-\u0DDF\u0DF2-\u0DF3\u0E01-\u0E3A\u0E40-\u0E4E\u0E50-\u0E59\u0E81-\u0E82\u0E84\u0E87-\u0E88\u0E8A\u0E8D\u0E94-\u0E97\u0E99-\u0E9F\u0EA1-\u0EA3\u0EA5\u0EA7\u0EAA-\u0EAB\u0EAD-\u0EB9\u0EBB-\u0EBD\u0EC0-\u0EC4\u0EC6\u0EC8-\u0ECD\u0ED0-\u0ED9\u0EDC-\u0EDF\u0F00\u0F18-\u0F19\u0F20-\u0F29\u0F35\u0F37\u0F39\u0F3E-\u0F47\u0F49-\u0F6C\u0F71-\u0F84\u0F86-\u0F97\u0F99-\u0FBC\u0FC6\u1000-\u1049\u1050-\u109D\u10A0-\u10C5\u10C7\u10CD\u10D0-\u10FA\u10FC-\u1248\u124A-\u124D\u1250-\u1256\u1258\u125A-\u125D\u1260-\u1288\u128A-\u128D\u1290-\u12B0\u12B2-\u12B5\u12B8-\u12BE\u12C0\u12C2-\u12C5\u12C8-\u12D6\u12D8-\u1310\u1312-\u1315\u1318-\u135A\u135D-\u135F\u1380-\u138F\u13A0-\u13F4\u1401-\u166C\u166F-\u167F\u1681-\u169A\u16A0-\u16EA\u16EE-\u16F0\u1700-\u170C\u170E-\u1714\u1720-\u1734\u1740-\u1753\u1760-\u176C\u176E-\u1770\u1772-\u1773\u1780-\u17D3\u17D7\u17DC-\u17DD\u17E0-\u17E9\u180B-\u180D\u1810-\u1819\u1820-\u1877\u1880-\u18AA\u18B0-\u18F5\u1900-\u191C\u1920-\u192B\u1930-\u193B\u1946-\u196D\u1970-\u1974\u1980-\u19AB\u19B0-\u19C9\u19D0-\u19D9\u1A00-\u1A1B\u1A20-\u1A5E\u1A60-\u1A7C\u1A7F-\u1A89\u1A90-\u1A99\u1AA7\u1B00-\u1B4B\u1B50-\u1B59\u1B6B-\u1B73\u1B80-\u1BF3\u1C00-\u1C37\u1C40-\u1C49\u1C4D-\u1C7D\u1CD0-\u1CD2\u1CD4-\u1CF6\u1D00-\u1DE6\u1DFC-\u1F15\u1F18-\u1F1D\u1F20-\u1F45\u1F48-\u1F4D\u1F50-\u1F57\u1F59\u1F5B\u1F5D\u1F5F-\u1F7D\u1F80-\u1FB4\u1FB6-\u1FBC\u1FBE\u1FC2-\u1FC4\u1FC6-\u1FCC\u1FD0-\u1FD3\u1FD6-\u1FDB\u1FE0-\u1FEC\u1FF2-\u1FF4\u1FF6-\u1FFC\u200C-\u200D\u203F-\u2040\u2054\u2071\u207F\u2090-\u209C\u20D0-\u20DC\u20E1\u20E5-\u20F0\u2102\u2107\u210A-\u2113\u2115\u2119-\u211D\u2124\u2126\u2128\u212A-\u212D\u212F-\u2139\u213C-\u213F\u2145-\u2149\u214E\u2160-\u2188\u2C00-\u2C2E\u2C30-\u2C5E\u2C60-\u2CE4\u2CEB-\u2CF3\u2D00-\u2D25\u2D27\u2D2D\u2D30-\u2D67\u2D6F\u2D7F-\u2D96\u2DA0-\u2DA6\u2DA8-\u2DAE\u2DB0-\u2DB6\u2DB8-\u2DBE\u2DC0-\u2DC6\u2DC8-\u2DCE\u2DD0-\u2DD6\u2DD8-\u2DDE\u2DE0-\u2DFF\u2E2F\u3005-\u3007\u3021-\u302F\u3031-\u3035\u3038-\u303C\u3041-\u3096\u3099-\u309A\u309D-\u309F\u30A1-\u30FA\u30FC-\u30FF\u3105-\u312D\u3131-\u318E\u31A0-\u31BA\u31F0-\u31FF\u3400-\u4DB5\u4E00-\u9FCC\uA000-\uA48C\uA4D0-\uA4FD\uA500-\uA60C\uA610-\uA62B\uA640-\uA66F\uA674-\uA67D\uA67F-\uA697\uA69F-\uA6F1\uA717-\uA71F\uA722-\uA788\uA78B-\uA78E\uA790-\uA793\uA7A0-\uA7AA\uA7F8-\uA827\uA840-\uA873\uA880-\uA8C4\uA8D0-\uA8D9\uA8E0-\uA8F7\uA8FB\uA900-\uA92D\uA930-\uA953\uA960-\uA97C\uA980-\uA9C0\uA9CF-\uA9D9\uAA00-\uAA36\uAA40-\uAA4D\uAA50-\uAA59\uAA60-\uAA76\uAA7A-\uAA7B\uAA80-\uAAC2\uAADB-\uAADD\uAAE0-\uAAEF\uAAF2-\uAAF6\uAB01-\uAB06\uAB09-\uAB0E\uAB11-\uAB16\uAB20-\uAB26\uAB28-\uAB2E\uABC0-\uABEA\uABEC-\uABED\uABF0-\uABF9\uAC00-\uD7A3\uD7B0-\uD7C6\uD7CB-\uD7FB\uF900-\uFA6D\uFA70-\uFAD9\uFB00-\uFB06\uFB13-\uFB17\uFB1D-\uFB28\uFB2A-\uFB36\uFB38-\uFB3C\uFB3E\uFB40-\uFB41\uFB43-\uFB44\uFB46-\uFBB1\uFBD3-\uFD3D\uFD50-\uFD8F\uFD92-\uFDC7\uFDF0-\uFDFB\uFE00-\uFE0F\uFE20-\uFE26\uFE33-\uFE34\uFE4D-\uFE4F\uFE70-\uFE74\uFE76-\uFEFC\uFF10-\uFF19\uFF21-\uFF3A\uFF3F\uFF41-\uFF5A\uFF66-\uFFBE\uFFC2-\uFFC7\uFFCA-\uFFCF\uFFD2-\uFFD7\uFFDA-\uFFDC]|\u[da-fA-F]{4})+)([\S\s]*)/;

  var LineTerminator = /^(?:\n\r|\r\n|\n|\r|\u2028|\u2029)([\S\s]*)/;

  var KEYWORDS = {
    "if": IDENT, "in": IDENT, "do": IDENT,
    "for": IDENT, "var": IDENT, "let": IDENT, "new": IDENT, "try": IDENT,
    "else": IDENT, "case": IDENT, "true": IDENT,
    "null": IDENT, "this": IDENT,
    "void": IDENT, "with": IDENT, "enum": IDENT,
    "break": IDENT, "catch": IDENT,
    "super": IDENT, "while": IDENT,
    "throw": IDENT, "class": IDENT,
    "const": IDENT, "false": IDENT,
    "delete": IDENT, "export": IDENT, "import": IDENT,
    "public": IDENT, "return": IDENT, "static": IDENT,
    "switch": IDENT, "typeof": IDENT,
    "default": IDENT, "extends": IDENT, "finally": IDENT,
    "package": IDENT, "private": IDENT,
    "debugger": IDENT, "continue": IDENT, "function": IDENT,
    "interface": IDENT, "protected": IDENT,
    "instanceof": IDENT, "implements": IDENT
  };
  var OPLIST = [
    "EOS", "ILLEGAL", "NOTFOUND",
    ".", ":", ";", ",", "...",

    "(", ")", "[", "]", "{", "}",

    "?", "#",

    "==", "===",

    "!", "!=", "!==",

    "++", "--", "<|",

    "+", "-", "*", "/", "%",

    "RELATIONAL_FIRST",
    "<", ">", "<=", ">=", "instanceof",
    "RELATIONAL_LAST",

    ">>", ">>>", "<<",

    "&", "|", "^", "~",

    "&&", "||",

    "ASSIGN_OP_FIRST",
    "=", "+=", "-=", "*=", "%=", "/=", ">>=", ">>>=", "<<=", "&=", "|=",
    "ASSIGN_OP_LAST",

    "delete", "typeof", "void", "break", "case", "catch", "continue", "debugger",
    "default", "do", "else", "finaly", "for", "function", "if", "in", "new",
    "return", "switch", "this", "throw", "try", "var", "while", "with",

    "abstract", "boolean", "byte", "char", "class", "const",
    "double","enum", "export", "extends", "final", "float",
    "goto", "implements", "import", "int", "interface", "long",
    "native", "package", "private", "protected", "public",
    "short", "static", "super", "synchronized", "throws",
    "transient", "volatile",

    "get", "set",

    "null", "false", "true", "NUMBER", "STRING", "IDENTIFIER"
  ];
  var OP = {};
  OPLIST.forEach(function(op, i) {
    OP[op] = i;
  });
  function isAssignOp(op) {
    return OP["ASSIGN_OP_FIRST"] < op && op < OP["ASSIGN_OP_LAST"];
  }
  function isRelationalOp(op) {
    return OP["RELATIONAL_FIRST"] < op && op < OP["RELATIONAL_LAST"];
  }
  function Lexer(source) {
    this.current = source;
    this.value = null;
    this.pos = 0;
    this.hasLineTerminatorBeforeNext = false;
  };
  Lexer.EOS = EOS;
  Lexer.ILLEGAL = ILLEGAL;
  Lexer.opToString = function(op) {
    return OPLIST[op];
  };
  Lexer.prototype.next = function(type) {
    var token = NOTFOUND;
    this.hasLineTerminatorBeforeNext = false;
    // remove white space
    do {
      this.current = this.current.replace(/^[\u0009\u000B\u000C\uFEFF\u0020\u00A0\u1680\u180E\u2000-\u200A\u202F\u205F\u3000]*/, "");
      if (this.current) {
        var m = null;
        if ((m = this.current.match(/^(\/[/*])([\S\s]*)/)) && m.length) {
          // skip comment
          if (m[1] === "//") {
            // SingleLineComment
            this.current = m[2].replace(/^[^\n\r\u2028\u2029]*/, "");
          } else if ((m = this.current.match(/^(\/\*[\s\S]*?\*\/)([\S\s]*)/)) && m.length) {
            // MultiLineComment
            var result = m[1];
            var right = m[2] || "";
            if (/[\n\r\u2028\u2029]/.test(result)) {
              // found LineTerminator
              this.hasLineTerminatorBeforeNext = true;
            }
            this.current = right;
          } else {
            token = ILLEGAL;
          }
        } else if ((m = this.current.match(/^(?:"((?:\\.|[^"])*)"|'((?:\\.|[^'])*)')([\S\s]*)/)) && m.length) {
          // scan string
          token = OP["STRING"];
          this.value = m[1] || m[2];
          this.current = m[3] || "";
        } else if ((m = this.current.match(/^(0x[\dA-Fa-f]+|\d+(?:\.\d*)?(?:[eE][-+]?\d+)?|\.\d+(?:[eE][-+]?\d+)?)([\S\s]*)/)) && m.length) {
          // scan number
          token = OP["NUMBER"];
          this.value = m[1];
          this.current = m[2] || "";
          if (this.current && (IdentifierStart.test(this.current) || /^\d/.test(this.current))) {
            token = ILLEGAL;
          }
        } else if ((m = this.current.match(/^(\.\.\.|===|!==|>>>=|>>>|>>=|<<=|\-\-|\+\+|>>|\|\||\|=|\^=|>=|==|<=|<\||&=|&&|!=|\+=|\/=|\-=|\*=|%=|<<|[#}|{^\[?=;:,(&!~+\-><\]\/\.*)%])([\S\s]*)/)) && m.length) {
          token = OP[m[1]];
          this.current = m[2] || "";
        } else if (IdentifierStart.test(this.current)) {
          // scan identifier
          m = this.current.match(IdentifierPart);
          this.value = m[1];
          if (type === IdentifyReservedWords) {
            token = KEYWORDS[this.value] === IDENT ? OP[this.value] : OP["IDENTIFIER"];
          } else if (type === IgnoreReservedWordsAndIdentifyGetterOrSetter) {
            token = this.value === 'get' ?
                    OP["get"] : this.value === 'set' ?
                    OP["set"] : OP["IDENTIFIER"];
          } else {
            token = OP["IDENTIFIER"];
          }
          this.current = m[2] || "";
        } else if ((m = this.current.match(LineTerminator)) && m.length) {
          // scan line terminator
          this.current = m[1] || "";
          this.hasLineTerminatorBeforeNext = true;
        } else {
          token = ILLEGAL;
        }
      } else {
        token = EOS;
      }
    } while(token === NOTFOUND);
    return token;
  };
  Lexer.prototype.scanRegExpLiteral = function(seenEqual) {
    var m = null;
    if (seenEqual) {
      if ((m = this.current.match(/^((?:\\.|\[(?:\\.|[^\]])*\]|[^\/])+)\/((?:[_$\da-zA-Z]|\\\u[\da-fA-F]{4})*)([\S\s]*)/)) && m.length) {
        this.value = '=' + m[1];
        this.flags = m[2] || "";
        this.current = m[3] || "";
        return true;
      }
    } else {
      if ((m = this.current.match(/^((?:\\.|\[(?:\\.|[^\]])*\]|[^\/])+)\/((?:[_$\da-zA-Z]|\\\u[\da-fA-F]{4})*)([\S\s]*)/)) && m.length) {
        this.value = m[1];
        this.flags = m[2] || "";
        this.current = m[3] || "";
        return true;
      }
    }
    return false;
  };

  function isBindingParseRequired(expr) {
    if (expr.type === 'Object' || expr.type === 'Array') {
      return !expr.sealed;
    }
    return false;
  }

  function Parser(source) {
    this.lexer = new Lexer(source);
    this.next();
  }

  Parser.prototype.parse = function() {
    var global = {
      type : "global",
      body : []
    };
    this.parseSourceElements(EOS, global);
    return global;
  };

  Parser.prototype.next = function(type) {
    return this.token = this.lexer.next(type || IdentifyReservedWords);
  };

  Parser.prototype.parseSourceElements = function(end, func) {
    while (this.token !== end) {
      func.body.push(this.parseStatementAndDeclaration());
    }
  };

  Parser.prototype.parseStatementAndDeclaration = function() {
    switch (this.token) {
      case OP["function"]:
        return this.parseFunctionDeclaration();

      case OP["const"]:
        return this.parseConstDeclaration();

      case OP["let"]:
        return this.parseLetDeclaration();

      default:
        return this.parseStatement();
    }
  };

  Parser.prototype.parseStatement = function() {
    if (this.token === ILLEGAL) {
      throw new Error("ILLEGAL");
    }
    switch (this.token) {
      case OP["{"]:
        // block
        return this.parseBlock();

      case OP["var"]:
        return this.parseVariableStatement();

      case OP[";"]:
        this.next();
        return {type:"EmptyStatement"};

      case OP["if"]:
        return this.parseIfStatement();

      case OP["do"]:
        return this.parseDoWhileStatement();

      case OP["while"]:
        return this.parseWhileStatement();

      case OP["for"]:
        return this.parseForStatement();

      case OP["continue"]:
        return this.parseContinueStatement();

      case OP["break"]:
        return this.parseBreakStatement();

      case OP["return"]:
        return this.parseReturnStatement();

      case OP["with"]:
        return this.parseWithStatement();

      case OP["switch"]:
        return this.parseSwitchStatement();

      case OP["throw"]:
        return this.parseThrowStatement();

      case OP["try"]:
        return this.parseTryStatement();

      case OP["debugger"]:
        this.next();
        this.expectSemicolon();
        return {type: "DebuggerStatement"};

      case OP["function"]:
        throw new Error("ILLEGAL");

      default:
        return this.parseExpressionOrLabelledStatement();
    }
    return null;
  };

  Parser.prototype.expectSemicolon = function() {
    if (this.lexer.hasLineTerminatorBeforeNext) {
      return true;
    }
    if (this.token === OP[";"]) {
      this.next();
      return true;
    }
    if (this.token === OP["}"] || this.token === EOS) {
      return true;
    }
    throw new Error("ILLEGAL");
  };

  Parser.prototype.expect = function(ex) {
    if (this.token !== ex) {
      throw new Error("ILLEGAL");
    }
    this.next();
  };

  Parser.prototype.parseFunctionDeclaration = function() {
    this.next();
    if (this.token === OP["IDENTIFIER"]) {
      return {
        type: "FunctionDeclaration",
        func: this.parseFunctionLiteral(DECL, 0)
      };
    } else {
      throw new Error("ILLEGAL");
    }
  };

  Parser.prototype.parseConstDeclaration = function() {
    var res = {
      type: "ConstDeclaration",
      body: []
    };
    this.parseDeclarations(res, 'const', true);
    this.expectSemicolon();
    return res;
  };

  Parser.prototype.parseLetDeclaration = function() {
    var res = {
      type: "LetDeclaration",
      body: []
    };
    this.parseDeclarations(res, 'let', true);
    this.expectSemicolon();
    return res;
  };

  Parser.prototype.parseBlock = function() {
    this.next();
    var block = {
      type: "Block",
      body: []
    };
    while (this.token !== OP["}"]) {
      block.body.push(this.parseStatementAndDeclaration());
    }
    this.next();
    return block;
  };

  Parser.prototype.parseVariableStatement = function() {
    var res = {
      type: "VariableStatement",
      body: []
    };
    this.parseDeclarations(res, 'var', true);
    this.expectSemicolon();
    return res;
  };

  Parser.prototype.parseDeclarations = function(res, token, accept_in) {
    do {
      this.next();
      var binding = this.parseBinding();
      if (this.token === OP["="]) {
        this.next();
        var expr = this.parseAssignmentExpression(accept_in);
        var decl = {
          type: "Declaration",
          key: binding,
          val: expr
        };
      } else {
        if (token === 'const') {
          throw new Error('ILLEGAL');
        }
        var decl = {
          type: "Declaration",
          key: binding
        };
      }
      res.body.push(decl);
    } while (this.token === OP[","]);
  };

  Parser.prototype.parseContinueStatement = function() {
    this.next();
    var stmt = {
      type: "ContinueStatement",
      label: null
    };
    if (this.token === OP["IDENTIFIER"] &&
        !this.lexer.hasLineTerminatorBeforeNext) {
      stmt.label = {
        type: "Identifier",
        value: this.lexer.value
      };
      this.next();
    }
    this.expectSemicolon();
    return stmt;
  };

  Parser.prototype.parseIfStatement = function() {
    this.next();
    this.expect(OP["("]);
    var expr = this.parseExpression(true);
    this.expect(OP[")"]);
    var stmt = this.parseStatement();
    var res = {
      type : "IfStatement",
      cond : expr,
      then : stmt
    };
    if (this.token === OP["else"]) {
      this.next();
      res["else"] = this.parseStatement();
    }
    return res;
  };

  Parser.prototype.parseDoWhileStatement = function() {
    this.next();
    var stmt = this.parseStatement();
    this.expect(OP["while"]);
    this.expect(OP["("]);
    var expr = this.parseExpression(true);
    this.expect(OP[")"]);
    this.expectSemicolon();
    return {
      type: "DoWhileStatement",
      body: stmt,
      cond: expr
    };
  };

  Parser.prototype.parseWhileStatement = function() {
    this.next();
    this.expect(OP["("]);
    var expr = this.parseExpression(true);
    this.expect(OP[")"]);
    return {
      type: "WhileStatement",
      cond: expr,
      body: this.parseStatement()
    };
  };

  function getDecl(token) {
    switch (token) {
      case 'let':
        return {
          type: 'LetDeclaration',
          body: []
        };
      case 'const':
        return {
          type: 'ConstDeclaration',
          body: []
        };
      case 'var':
        return {
          type: 'VariableStatement',
          body: []
        };
    }
  }

  Parser.prototype.parseForStatement = function() {
    this.next();
    this.expect(OP["("]);
    if (this.token !== OP[";"]) {
      if (this.token === OP["var"] || this.token === OP["const"] || this.token === OP['let']) {
        var token = Lexer.opToString(this.token);
        var init = getDecl(token);
        // ForDecl allow non-initialized const declaration
        this.parseDeclarations(init, 'let', false);
        if (this.token === OP["in"] || (this.token === OP['IDENTIFIER'] && this.lexer.value === 'of')) {
          var type = (this.token === OP['in']) ? 'ForInStatement' : 'ForOfStatement';
          if (init.body.length !== 1) {
            throw new Error("ILLEGAL");
          }
          // reject
          // for (let i = 20 in []);
          if (token !== 'var' && init.body[0].val) {
            throw new Error("ILLEGAL");
          }
          this.next();
          var enumerable = this.parseExpression(true);
          this.expect(OP[")"]);
          var body = this.parseStatement();
          return {
            type: type,
            init: init,
            enumerable: enumerable,
            body: body
          };
        }
      } else {
        var save = this.save();
        var initExpr = this.parseExpression(false);
        var init = {
          type: "ExpressionStatement",
          expr: initExpr
        };
        // currently, using No in expression in for-of statement
        //
        // because,
        //
        //   for (expr in [] of []);
        //
        // is ambiguous
        if (this.token === OP["in"] || (this.token === OP['IDENTIFIER'] && this.lexer.value === 'of')) {
          var type = (this.token === OP['in']) ? 'ForInStatement' : 'ForOfStatement';
          if (isBindingParseRequired(initExpr)) {
            var save2 = this.save();
            this.restore(save);
            try {
              init.initExpr = this.parseAssignmentPattern();
            } catch (e) {
              this.restore(save2);
            }
          }
          this.next();
          var enumerable = this.parseExpression(true);
          this.expect(OP[")"]);
          var body = this.parseStatement();
          return {
            type: type,
            init: init,
            enumerable: enumerable,
            body: body
          };
        }
      }
    }

    this.expect(OP[";"]);

    var cond = null;
    if (this.token === OP[";"]) {
      this.next();
    } else {
      cond = this.parseExpression(true);
      this.expect(OP[";"]);
    }

    var next = null;
    if (this.token === OP[")"]) {
      this.next();
    } else {
      var nextExpr = this.parseExpression(true);
      next = {
        type: "ExpressionStatement",
        expr: nextExpr
      };
      this.expect(OP[")"]);
    }

    var body = this.parseStatement();
    return {
      type: "ForStatement",
      init: init,
      cond: cond,
      next: next,
      body: body
    };
  };

  Parser.prototype.parseBreakStatement = function() {
    this.next();
    var stmt = {
      type: "BreakStatement",
      label: null
    };
    if (this.token === OP["IDENTIFIER"] &&
        !this.lexer.hasLineTerminatorBeforeNext) {
      stmt.label = {
        type: "Identifier",
        value: this.lexer.value
      };
      this.next();
    }
    this.expectSemicolon();
    return stmt;
  };

  Parser.prototype.parseReturnStatement = function() {
    var stmt = {
      type: "ReturnStatement",
      expr: null
    };
    this.next();
    if (!this.lexer.hasLineTerminatorBeforeNext &&
        this.token !== OP[";"] &&
        this.token !== OP["}"] &&
        this.token !== EOS) {
      stmt.expr = this.parseExpression(true);
    } else {
      stmt.expr = null;
    }
    this.expectSemicolon();
    return stmt;
  };

  Parser.prototype.parseWithStatement = function() {
    this.next();
    this.expect(OP["("]);
    var expr = this.parseExpression(true);
    this.expect(OP[")"]);
    return {
      type: "WithStatement",
      body: this.parseStatement(),
      expr: expr
    };
  };

  Parser.prototype.parseSwitchStatement = function() {
    this.next();
    this.expect(OP["("]);
    var expr = this.parseExpression(true);
    var res = {
      type: "SwitchStatement",
      expr: expr,
      clauses: []
    };
    this.expect(OP[")"]);
    this.expect(OP["{"]);

    while (this.token !== OP["}"]) {
      var clause = this.parseCaseClause();
      res.clauses.push(clause);
    }
    this.next();
    return res;
  };

  Parser.prototype.parseCaseClause = function() {
    var clause = {
      type: "Caluse",
      body: []
    };
    if (this.token === OP["case"]) {
      this.next();
      var expr = this.parseExpression(true);
      clause.kind = "case";
      clause.expr = expr;
    } else {
      this.expect(OP["default"]);
      clause.kind = "default";
    }

    this.expect(OP[":"]);

    while (this.token !== OP["}"] &&
           this.token !== OP["case"] &&
           this.token !== OP["default"]) {
      var stmt = this.parseStatement();
      clause.body.push(stmt);
    }
    return clause;
  };

  Parser.prototype.parseThrowStatement = function() {
    this.next();
    if (this.lexer.hasLineTerminatorBeforeNext) {
      throw new Error("ILLEGAL");
    }
    var expr = this.parseExpression(true);
    this.expectSemicolon();
    return {type:"ThrowStatement", expr: expr};
  };

  Parser.prototype.parseTryStatement = function() {
    var hasCatchOrFinally = false;
    this.next();

    if (this.token !== OP["{"]) {
      throw new Error("ILLEGAL");
    }

    var res = {
      type : "TryStatement",
      block: this.parseBlock()
    };

    if (this.token === OP["catch"]) {
      hasCatchOrFinally = true;
      this.next();
      this.expect(OP["("]);
      var binding = this.parseBinding();
      this.expect(OP[")"]);
      if (this.token !== OP["{"]) {
        throw new Error("ILLEGAL");
      }
      var block = this.parseBlock();
      res.catchBlock = {
        name: binding,
        block: block
      };
    }

    if (this.token === OP["finally"]) {
      hasCatchOrFinally = true;
      this.next();
      if (this.token !== OP["{"]) {
        throw new Error("ILLEGAL");
      }
      var block = this.parseBlock();
      res.finallyBlock = {
        block: block
      };
    }

    if (!hasCatchOrFinally) {
      throw new Error("ILLEGAL");
    }

    return res;
  };

  Parser.prototype.parseExpressionOrLabelledStatement = function() {
    if (this.token === OP["IDENTIFIER"]) {
      var expr = this.parseExpression(true);
      if (this.token === OP[":"] &&
          expr.type === "Identifier") {
        this.next();
        return {
          type: "LabelledStatement",
          expr: expr,
          body: this.parseStatement()
        };
      }
    } else {
      var expr = this.parseExpression(true);
    }
    this.expectSemicolon();
    return {
      type: "ExpressionStatement",
      expr: expr
    };
  };

  Parser.prototype.parseExpression = function(containsIn) {
    var result = this.parseAssignmentExpression(containsIn);
    while (this.token === OP[","]) {
      this.next();
      var right = this.parseAssignmentExpression(containsIn);
      result = {
        type: "BinaryExpression",
        op  : ",",
        left: result,
        right: right
      };
    }
    return result;
  };

  Parser.prototype.parseAssignmentExpression = function(containsIn) {
    var save = this.save();
    var result = this.parseConditionalExpression(containsIn);
    if (!isAssignOp(this.token)) {
      return result;
    }
    if (this.token === OP['='] && isBindingParseRequired(result)) {
      var save2 = this.save();
      this.restore(save);
      try {
        result = this.parseAssignmentPattern();
      } catch (e) {
        this.restore(save2);
      }
    }
    var op = Lexer.opToString(this.token);
    this.next();
    var right = this.parseAssignmentExpression(containsIn);
    return {
      type: "Assignment",
      op: op,
      left: result,
      right: right
    };
  };

  Parser.prototype.parseConditionalExpression = function(containsIn) {
    var result = this.parseBinaryExpression(containsIn, 9);
    if (this.token === OP["?"]) {
      this.next();
      var left = this.parseAssignmentExpression(true);
      this.expect(OP[":"]);
      var right = this.parseAssignmentExpression(containsIn);
      result = {
        type: "ConditionalExpression",
        cond: result,
        left: left,
        right: right
      };
    }
    return result;
  };

  Parser.prototype.parseBinaryExpression = function(containsIn, prec) {
    var left = this.parseUnaryExpression();
    while (this.token === OP["*"] ||
           this.token === OP["/"] ||
           this.token === OP["%"]) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseUnaryExpression();
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 1) return left;

    while (this.token === OP["+"] || this.token === OP["-"]) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 0);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 2) return left;

    while (this.token === OP["<<"] ||
           this.token === OP[">>>"] ||
           this.token === OP[">>"]) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 1);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 3) return left;

    while (isRelationalOp(this.token) ||
           (containsIn && this.token === OP["in"])) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 2);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 4) return left;

    while (this.token === OP["==="] ||
           this.token === OP["!=="] ||
           this.token === OP["=="] ||
           this.token === OP["!="] ||
           (!this.lexer.hasLineTerminatorBeforeNext &&
            this.token === OP["IDENTIFIER"] &&
            (this.lexer.value === "is" || this.lexer.value === "isnt"))) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 3);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 5) return left;

    while (this.token === OP["&"]) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 4);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 6) return left;

    while (this.token === OP["^"]) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 5);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 7) return left;

    while (this.token === OP["|"]) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 6);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 8) return left;

    while (this.token === OP["&&"]) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 7);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    if (prec < 9) return left;

    while (this.token === OP["||"]) {
      var op = Lexer.opToString(this.token);
      this.next();
      var right = this.parseBinaryExpression(containsIn, 8);
      left = { type: "BinaryExpression",
        op: op, left: left, right: right
      };
    }
    return left;
  };

  Parser.prototype.parseUnaryExpression = function() {
    var op = Lexer.opToString(this.token);
    switch (this.token) {
      case OP["void"]:
      case OP["!"]:
      case OP["typeof"]:
      case OP["~"]:
      case OP["+"]:
      case OP["-"]:
      case OP["delete"]:
        this.next();
        var expr = this.parseUnaryExpression();
        return {type: "UnaryExpression", op: op, expr: expr};

      case OP["++"]:
      case OP["--"]:
        this.next();
        var expr = this.parseMemberExpression();
        return {type: "UnaryExpression", op: op, expr: expr};

      default:
        return this.parsePostfixExpression();
    }
  };

  Parser.prototype.parsePostfixExpression = function() {
    var expr = this.parseMemberExpression(true);
    if (!this.lexer.hasLineTerminatorBeforeNext &&
        (this.token === OP["++"] || this.token === OP["--"])) {
      expr = {
        type:"PostfixExpression",
        op: Lexer.opToString(this.token),
        expr: expr
      };
      this.next();
    }
    return expr;
  };

  Parser.prototype.parseMemberExpression = function(allowCall) {
    var expr = null;
    if (this.token === OP["new"]) {
      this.next();
      var target = this.parseMemberExpression(false);
      expr = {
        type: "NewCall",
        target: target,
        args: []
      };
      if (this.token === OP["("]) {
        this.parseArguments(expr);
      }
    } else if (this.token === OP["super"]) {
      expr = this.parseSuper();
    } else {
      expr = this.parsePrimaryExpression();
    }
    while (true) {
      switch (this.token) {
        case OP["["]:
          this.next();
          var index = this.parseExpression(true);
          expr = {
            type: "PropertyAccess",
            target: expr,
            key: index
          };
          this.expect(OP["]"]);
          break;

        case OP["."]:
          this.next(IgnoreReservedWords);
          if (this.token !== OP["IDENTIFIER"]) {
            throw new Error("ILLEGAL");
          }
          var index = {
            type: "Identifier",
            value: this.lexer.value
          };
          this.next();
          expr = {
            type: "PropertyAccess",
            target: expr,
            key: index
          };
          break;

        case OP["("]:
          if (allowCall) {
            expr = {
              type: "FuncCall",
              target: expr,
              args: []
            };
            this.parseArguments(expr);
          } else {
            return expr;
          }
          break;

        case OP['<|']: {
          this.next();
          var proto = this.parseTriangleLiteral();
          expr = {
            type: "TriangleLiteral",
            target: expr,
            proto: proto
          };
          break;
        }

        default:
          return expr;
      }
    }
  };

  Parser.prototype.parseSuper = function() {
    this.next();
    if (this.token === OP["["]) {
      this.next();
      var index = this.parseExpression(true);
      var expr = {
        type: "PropertyAccess",
        target: { type: "Super" },
        key: index
      };
      this.expect(OP["]"]);
      return expr;
    } else if (this.token === OP["."]) {
      this.next(IgnoreReservedWords);
      if (this.token !== OP["IDENTIFIER"]) {
        throw new Error("ILLEGAL");
      }
      var index = {
        type: "Identifier",
        value: this.lexer.value
      };
      this.next();
      return {
        type: "PropertyAccess",
        target: { type: "Super" },
        key: index
      };
    } else if (this.token === OP["("]) {
      var expr = {
        type: "FuncCall",
        target: { type: "Super" },
        args: []
      };
      this.parseArguments(expr);
      return expr;
    }
    throw new Error("ILLEGAL");
  };

  Parser.prototype.parseTriangleLiteral = function() {
    switch (this.token) {
      case OP["function"]:
        this.next();
        return this.parseFunctionLiteral(EXP, 0);

      case OP["NUMBER"]:
        var value = this.lexer.value;
        this.next();
        return {
          type: "Number",
          value: value
        };

      case OP["STRING"]:
        var value = this.lexer.value;
        this.next();
        return {
          type: "String",
          value: value
        };

      case OP["/"]:
        return this.parseRegExpLiteral(false);

      case OP["/="]:
        return this.parseRegExpLiteral(true);

      case OP["#"]: {  // Sealed Literal
        this.next();
        if (this.token == OP["["]) {
          return this.parseArrayLiteralOrComprehension(true);
        } else if (this.token == OP["{"]) {
          return this.parseObjectLiteral(true);
        }
        throw new Error("ILLEGAL");
      }

      case OP["["]:
        return this.parseArrayLiteralOrComprehension(false);

      case OP["{"]:
        return this.parseObjectLiteral(false);

      default:
        throw new Error("ILLEGAL");

    }
  };

  Parser.prototype.parsePrimaryExpression = function() {
    switch (this.token) {
      case OP["function"]:
        this.next();
        if (this.token === OP['*']) {
          this.next();
          var ret = this.parseFunctionLiteral(EXP, 0);
          ret.type = 'GeneratorExpression';
          return ret;
        }
        return this.parseFunctionLiteral(EXP, 0);

      case OP["this"]:
        this.next();
        return { type: "This" };

      case OP["IDENTIFIER"]:
        var value = this.lexer.value;
        this.next();
        return {
          type: "Identifier",
          value: value
        };

      case OP["null"]:
        this.next();
        return { type: "Null" };

      case OP["true"]:
        this.next();
        return { type: "True" };

      case OP["false"]:
        this.next();
        return { type: "False" };

      case OP["NUMBER"]:
        var value = this.lexer.value;
        this.next();
        return {
          type: "Number",
          value: value
        };

      case OP["STRING"]:
        var value = this.lexer.value;
        this.next();
        return {
          type: "String",
          value: value
        };

      case OP["/"]:
        return this.parseRegExpLiteral(false);

      case OP["/="]:
        return this.parseRegExpLiteral(true);

      case OP["#"]: {  // Sealed Literal
        this.next();
        if (this.token == OP["["]) {
          return this.parseArrayLiteralOrComprehension(true);
        } else if (this.token == OP["{"]) {
          return this.parseObjectLiteral(true);
        }
        throw new Error("ILLEGAL");
      }

      case OP["["]:
        return this.parseArrayLiteralOrComprehension(false);

      case OP["{"]:
        return this.parseObjectLiteral(false);

      case OP["("]:
        this.next();
        var result = this.parseExpression(true);
        this.expect(OP[")"]);
        return result;

      default:
        throw new Error("ILLEGAL");
    }
  };

  Parser.prototype.parseArguments = function(func) {
    this.next();
    while (this.token !== OP[")"]) {
      if (this.token === OP["..."]) {
        this.next();
        var expr = {
          type: "AssignmentRestExpression",
          expr: this.parseAssignmentExpression(true)
        };
      } else {
        var expr = this.parseAssignmentExpression(true);
      }
      func.args.push(expr);
      if (this.token !== OP[")"]) {
        this.expect(OP[","]);
      }
    }
    this.next();
    return func;
  };

  Parser.prototype.parseRegExpLiteral = function(containsEq) {
    if (this.lexer.scanRegExpLiteral(containsEq)) {
      var expr = {
        type: "RegExp",
        value: this.lexer.value,
        flags: this.lexer.flags
      };
      this.next();
      return expr;
    } else {
      throw new Error("ILLEGAL");
    }
  };

  Parser.prototype.parseArrayLiteral = function(literal) {
    while (this.token !== OP["]"]) {
      if (this.token === OP[","]) {
        literal.items.push(null);
      } else if (this.token === OP["..."]) {
        this.next();
        literal.items.push({
          type: "RestElement",
          expr: this.parseAssignmentExpression(true)
        });
        if (this.token !== OP["]"]) {
          throw new Error("ILLEGAL");
        }
        break;
      } else {
        literal.items.push(this.parseAssignmentExpression(true));
      }
      if (this.token !== OP["]"]) {
        this.expect(OP[","]);
      }
    }
    this.next();
    return literal;
  };

  Parser.prototype.parseComprehensionForList = function() {
    var result = [];
    while (this.token === OP['for']) {
      this.next();
      this.expect(OP['(']);
      var expr = this.parseExpression(true);
      var init = {
        type: "ExpressionStatement",
        expr: expr
      };
      if (this.token !== OP['IDENTIFIER'] || this.lexer.value !== 'of') {
          throw new Error("ILLEGAL");
      }
      this.next();
      var enumerable = this.parseExpression(true);
      this.expect(OP[")"]);
      result.push({
        type: 'ComprehensionBlock',
        left: init,
        right: enumerable,
      });
    }
    return result;
  };

  Parser.prototype.parseArrayLiteralOrComprehension = function(sealed) {
    this.next();

    // First Expression
    if (this.token === OP[']'] || this.token === OP[','] || this.token === OP['...']) {
      return this.parseArrayLiteral({
        type: "Array",
        sealed: sealed,
        items: []
      });
    }

    var save = this.save();
    var first = this.parseExpression(true);

    if (this.token !== OP['for']) {
      // ArrayLiteral path
      this.restore(save);
      return this.parseArrayLiteral({
        type: "Array",
        sealed: sealed,
        items: []
      });
    }

    // parsing ArrayComprehension
    var comprehensionForList = this.parseComprehensionForList();

    var filter = null;
    if (this.token === OP['if']) {
      this.next();
      this.expect(OP['(']);
      filter = this.parseExpression(true);
      this.expect(OP[')']);
    }

    this.expect(OP[']']);
    return {
      type: 'ArrayComprehension',
      expression: first,
      comprehensions: comprehensionForList,
      filter: filter
    };
  };

  Parser.prototype.parseObjectLiteral = function(sealed) {
    var literal = {
      type: "Object",
      sealed: sealed,
      values: [],
      accessors : []
    };
    this.next(IgnoreReservedWordsAndIdentifyGetterOrSetter);
    while (this.token !== OP["}"]) {
      if (this.token === OP["get"] || this.token === OP["set"]) {
        var is_getter = this.token === OP["get"];
        this.next(IgnoreReservedWords);
        if (this.token === OP[":"]) {
          this.next();
          literal.values.push({
            key: is_getter ? "get" : "set",
            val: this.parseAssignmentExpression(true)
          });
        } else if (this.token === OP["("]) {
          literal.values.push({
            key: is_getter ? "get" : "set",
            val: this.parseFunctionLiteral(EXP, 0)
          });
        } else {
          if (this.token === OP["IDENTIFIER"] ||
              this.token === OP["STRING"] ||
              this.token === OP["NUMBER"]) {
            var name = this.lexer.value;
            this.next();
            var expr = this.parseFunctionLiteral(EXP, is_getter ? GETTER : SETTER);
            literal.accessors.push({
              type: "Accessor",
              kind: is_getter ? "getter" : "setter",
              name: name,
              func: expr
            });
          } else {
            throw new Error("ILLEGAL");
          }
        }
      } else if (this.token === OP["IDENTIFIER"] ||
                 this.token === OP["STRING"] ||
                 this.token === OP["NUMBER"]) {
        var key = this.lexer.value;
        this.next();
        if (this.token === OP[":"]) {
          this.next();
          literal.values.push({
            key: key,
            val: this.parseAssignmentExpression(true)
          });
        } else if (this.token === OP["("]) {
          literal.values.push({
            key: key,
            val: this.parseFunctionLiteral(EXP, 0)
          });
        } else {
          throw new Error("ILLEGAL");
        }
      } else {
        throw new Error("ILLEGAL");
      }
      if (this.token !== OP["}"]) {
        if (this.token !== OP[","]) {
          throw new Error("ILLEGAL");
        }
        this.next(IgnoreReservedWordsAndIdentifyGetterOrSetter);
      }
    }
    this.next();
    return literal;
  };

  Parser.prototype.parseFunctionLiteral = function(kind, getterOrSetter) {
    var literal = {
      type: "Function",
      kind: kind,
      params: [],
      body: []
    };
    if (!getterOrSetter) {
      if (this.token === OP["IDENTIFIER"]) {
        literal.name = this.lexer.value;
        this.next();
      }
      this.expect(OP["("]);
      while (this.token !== OP[")"]) {
        if (this.token === OP["..."]) {
          this.next();
          if (this.token !== OP["IDENTIFIER"]) {
            throw new Error("ILLEGAL");
          }
          literal.params.push({
            type: "RestParameter",
            name: {
              type: "Identifier",
              value: this.lexer.value
            }
          });
          this.next();
          if (this.token !== OP[")"]) {
            throw new Error("ILLEGAL");
          }
          break;
        } else {
          literal.params.push(this.parseBindingElement());
        }
        if (this.token !== OP[")"]) {
          this.expect(OP[","]);
        }
      }
      this.next();
    } else {
      // getter or setter
      if (getterOrSetter === GETTER) {
        this.expect(OP["("]);
        this.expect(OP[")"]);
      } else {
        this.expect(OP["("]);
        literal.params.push(this.parseBinding());
        this.expect(OP[")"]);
      }
    }

    this.expect(OP["{"]);
    this.parseSourceElements(OP["}"], literal);
    this.next();
    return literal;
  };

  Parser.prototype.parseBindingPattern = function() {
    if (this.token === OP["{"]) {
      // ObjectBindingPattern
      var pattern = {
        type: "ObjectBindingPattern",
        patterns: []
      };
      this.next(IgnoreReservedWords);
      while (this.token !== OP["}"]) {
        var ident;
        if (this.token === OP["IDENTIFIER"]) {
          var key = this.lexer.value;
          this.next();
          ident = {
            type: "Identifier",
            value: key
          };
        } else if (this.token === OP["STRING"] ||
                   this.token === OP["NUMBER"]) {
          var key = this.lexer.value;
          this.next();
          ident = {
            type: "Identifier",
            value: key
          };
          if (this.token !== OP[":"]) {
            throw new Error("ILLEGAL");
          }
        } else {
          throw new Error("ILLEGAL");
        }
        if (this.token === OP[":"]) {
          this.next();
          pattern.patterns.push({
            type: "BindingProperty",
            key: ident,
            val: this.parseBindingElement()
          });
        } else if (this.token === OP["="]) {
          // SingleNameBinding with Initializer
          this.next();
          pattern.patterns.push({
            type: "SingleNameBinding",
            key: ident,
            val: this.parseAssignmentExpression(true)
          });
        } else {
          pattern.patterns.push(ident);
        }
        if (this.token !== OP["}"]) {
          if (this.token !== OP[","]) {
            throw new Error("ILLEGAL");
          }
          this.next(IgnoreReservedWords);
        }
      }
      this.next();
      return pattern;
    } else if (this.token === OP["["]) {
      // ArrayBindingPattern
      this.next();
      var pattern = {
        type: "ArrayBindingPattern",
        patterns: []
      };
      while (this.token !== OP["]"]) {
        if (this.token === OP[","]) {
          pattern.patterns.push({ type: "Elision" });
        } else if (this.token === OP["..."]) {
          this.next();
          if (this.token !== OP["IDENTIFIER"]) {
            throw new Error("ILLEGAL");
          }
          pattern.patterns.push({
            type: "BindingRestElement",
            name: {
              type: "Identifier",
              value: this.lexer.value
            }
          });
          this.next();
          if (this.token !== OP["]"]) {
            throw new Error("ILLEGAL");
          }
          break;
        } else {
          pattern.patterns.push(this.parseBindingElement());
        }
        if (this.token !== OP["]"]) {
          this.expect(OP[","]);
        }
      }
      this.next();
      return pattern;
    } else {
      throw new Error("ILLEGAL");
    }
  };

  Parser.prototype.parseBinding = function() {
    if (this.token === OP["IDENTIFIER"]) {
      var val = this.lexer.value;
      this.next();
      return {
        type: "Identifier",
        value: val
      };
    } else {
      return this.parseBindingPattern();
    }
  };

  Parser.prototype.parseBindingElement = function() {
    if (this.token === OP["IDENTIFIER"]) {
      var val = this.lexer.value;
      this.next();
      var ident = {
        type: "Identifier",
        value: val
      };
      if (this.token === OP["="]) {
        this.next();
        return {
          type: "SingleNameBinding",
          key: ident,
          val: this.parseAssignmentExpression(true)
        };
      }
      return ident;
    } else {
      return this.parseBindingPattern();
    }
  };

  Parser.prototype.parseAssignmentPattern = function() {
    if (this.token === OP["{"]) {
      // ObjectAssignmentPattern
      var pattern = {
        type: "AssignmentPattern",
        pattern: {
          type: "ObjectAssignmentPattern",
          patterns: []
        }
      };
      this.next(IgnoreReservedWords);
      while (this.token !== OP["}"]) {
        var ident;
        if (this.token === OP["IDENTIFIER"]) {
          var key = this.lexer.value;
          this.next();
          ident = {
            type: "Identifier",
            value: key
          };
        } else if (this.token === OP["STRING"] ||
                   this.token === OP["NUMBER"]) {
          var key = this.lexer.value;
          this.next();
          ident = {
            type: "Identifier",
            value: key
          };
          if (this.token !== OP[":"]) {
            throw new Error("ILLEGAL");
          }
        } else {
          throw new Error("ILLEGAL");
        }
        if (this.token === OP[":"]) {
          this.next();
          var save = this.save();
          var target = this.parseMemberExpression();
          if (isBindingParseRequired(target)) {
            var save2 = this.save2();
            this.restore(save);
            try {
              target = this.parseAssignmentPattern();
            } catch (e) {
              this.restore(save2);
            }
          }
          pattern.pattern.patterns.push({
            type: "AssignmentProperty",
            key: ident,
            target: target
          });
        } else {
          pattern.pattern.patterns.push(ident);
        }
        if (this.token !== OP["}"]) {
          if (this.token !== OP[","]) {
            throw new Error("ILLEGAL");
          }
          this.next(IgnoreReservedWords);
        }
      }
      this.next();
      return pattern;
    } else if (this.token === OP["["]) {
      // ArrayAssignmentPattern
      this.next();
      var pattern = {
        type: "AssignmentPattern",
        pattern: {
          type: "ArrayAssignmentPattern",
          patterns: []
        }
      };
      while (this.token !== OP["]"]) {
        if (this.token === OP[","]) {
          pattern.pattern.patterns.push({ type: "Elision" });
        } else if (this.token === OP["..."]) {
          this.next();
          var save = this.save();
          var target = this.parseMemberExpression();
          if (isBindingParseRequired(target)) {
            var save2 = this.save2();
            this.restore(save);
            try {
              target = this.parseAssignmentPattern();
            } catch (e) {
              this.restore(save2);
            }
          }
          pattern.pattern.patterns.push({
            type: "AssignmentRestElement",
            expr: target
          });
          this.next();
          if (this.token !== OP["]"]) {
            throw new Error("ILLEGAL");
          }
          break;
        } else {
          var save = this.save();
          var target = this.parseMemberExpression();
          if (isBindingParseRequired(target)) {
            var save2 = this.save2();
            this.restore(save);
            try {
              target = this.parseAssignmentPattern();
            } catch (e) {
              this.restore(save2);
            }
          }
          pattern.pattern.patterns.push(target);
        }
        if (this.token !== OP["]"]) {
          this.expect(OP[","]);
        }
      }
      this.next();
      return pattern;
    } else {
      throw new Error("ILLEGAL");
    }
  };

  Parser.prototype.save = function() {
    return {
      token: this.token,
      current: this.lexer.current,
      value: this.lexer.value,
      pos: this.lexer.pos,
      flags: this.lexer.flags,
      hasLineTerminatorBeforeNext: this.lexer.hasLineTerminatorBeforeNext
    };
  };

  Parser.prototype.restore = function(obj) {
    this.token = obj.token;
    this.lexer.current = obj.current;
    this.lexer.value = obj.value;
    this.lexer.pos = obj.pos;
    this.lexer.flags = obj.flags;
    this.lexer.hasLineTerminatorBeforeNext = obj.hasLineTerminatorBeforeNext;
  };

  exp.Parser = Parser;
  exp.Lexer = Lexer;
})(this);

// print(JSON.stringify(new Parser("if(a||!a);").parse()));
