function toggle(field) {
  field = "#" + field;
  jQuery(field + "-YES, " + field + "-NO").toggle();
  jQuery(field).val(jQuery(field + "-YES").is(":visible"));
}
function toggleMain(me) {
  var was=jQuery(me).is(":visible");
  jQuery("#main, #help, #menu").hide();
  jQuery(was ? "#main" : me).show();
}
function commafy(n) {
  if(isNaN(n)) return '0.00';
  n=parseFloat(n).toFixed(2).split(".");
  n[0] = n[0].replace(/\B(?=(\d{3})+(?!\d))/g, ",");
  return n.join(".");
}

//jQuery('body').css('font-size',jQuery(window).height()/100); // sets em=h/100 -- maybe do width instead

jQuery("#which, #help").addClass("popup");
jQuery("#help-close, #which-close").addClass("popup-close");
jQuery("#help-link").click(function() {jQuery("fieldset#help").show();});
//jQuery("#edit-acct-showhelp").click(function() {toggleMain("#help");});
//jQuery("#edit-acct-showmenu").click(function() {toggleMain("#menu");});
jQuery(".popup-close").click(function() {jQuery(this).parents("fieldset").hide();});
jQuery("#which-cancel").click(function() {jQuery("fieldset#which").hide();});
// jQuery("#idcard").click(function() {window.open("Advance-ID-Card", "tempcard","height=250,width=600,left=100,location=0,menubar=0,resizable=0,scrollbars=0,status=0,toolbar=0");});
var indexZ = 2;
jQuery("#edit-acct-index a").mouseover(function() {
  var detail = jQuery("#" + this.id + "-detail");
  indexZ++;
  detail.css("zIndex", indexZ); // hiding the others fails here (as does detail.zIndex(indexZ))
  detail.show();
});
jQuery(".index-detail").click(function() {jQuery("#edit-acct-index, .index-detail").hide();});
jQuery("#logo img").mouseover(function() {
  /* (FAILS) if (jQuery("#edit-acct-index").is(":visible")) {
    jQuery("#edit-acct-index, .index-detail").hide();
  } else jQuery("#edit-acct-index").show(); */
  jQuery("#edit-acct-index").toggle();
});