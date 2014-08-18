
Laconic code for generating HTML DOM from javascript:
$.el.table(
  $.el.tr(
    $.el.th('first name'),
    $.el.th('last name')),
  $.el.tr(
    $.el.td('Joe'),
    $.el.td('Stelmach'))
).appendTo(document.body);

I quite like that - it might be nice to do something similar inside C++ with xo, probably with a C++ DSL.
