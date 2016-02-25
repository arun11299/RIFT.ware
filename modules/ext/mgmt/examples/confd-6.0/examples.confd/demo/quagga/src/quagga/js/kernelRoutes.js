KernelRoutes = {
  init: function() {
    var url = ConfD.baseUrl+'/kernelRoutes.html';
    new Ajax.Updater({success: 'containerHolder'}, url, {onComplete: function (transport) {
      KernelRoutes.start($('routeForm'));
    }});
  },

  start: function(form) {
    var elements = $$('#' + form.id + ' .optionalField');
    var undef = '(undef)';
    var doUndef = function (elem) { elem.style.color = 'blue'; elem.value = undef; }
    var change = function() { this.fieldTouched = 1; };
    var focus = function() { if (! this.fieldTouched) { this.style.color = 'black'; this.value = ''; } };
    var blur = function() { if (! this.fieldTouched) doUndef(this); };
    elements.each(function (elem) {
      elem.fieldTouched  = 0;
      doUndef(elem);
      elem.addEventListener('focus', focus, false);
      elem.addEventListener('change', change, false);
      elem.addEventListener('blur', blur, false);
    });
  },

  click: function(form) {
    var params = [{name: 'operation', value: form.operation.value}];
    if (form.operation.value == 'add' || form.operation.value == 'delete') {
      var pref = form.operation.value == 'add' ? 'route-to-add/' : 'route-to-delete/';
      params.push({name: pref + 'type', value: $('net').checked ? 'net' : 'host'});
      params.push({name: pref + 'destination', value: form.destination.value});
      ['gateway', 'mask', 'metric', 'iface'].each(function (name) {
        if (form[name].fieldTouched)
          params.push({name: pref + name, value: form[name].value});
      });
    }

    Maapi.triggerAction(-1, Container.currentKeypath, params, function(res) {
      if (Maapi.isErrorResult(res))
        Tracer.informUser(res.error);
      else {
        var contentPane = $('contentPane');
        contentPane.innerHTML = '';
        if (res.length > 0) {
          contentPane.appendChild(H1('Kernel routes'));
          var tbody = TBODY(TR(TH('destination'), TH('gateway'), TH('mask'), TH('flags'), TH('metric'), TH('iface')));
          for (var i = 0; i < res.length; i += 7)
            // (there is 7th element - type; ignored)
            tbody.appendChild(TR(TD(res[i].value), TD(res[i+1].value), TD(res[i+2].value), TD(res[i+3].value),
                                 TD(res[i+4].value), TD(res[i+5].value)));
          contentPane.appendChild(TABLE({border: 1, id: 'routeTable'}, tbody));
        }
        contentPane.appendChild(P('Done.'));
      }
    });
    return false;
  }
}
