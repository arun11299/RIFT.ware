angular.module('uesim',[])
    .controller('uesimCtrl',function(){

    })
    .directive('uesimulator', function(){
        return{
            restrict: 'C',
            link: function (scope, element, attr) {
                //----- Mouse Intent Plug In
                !function(e){
                    e.fn.mouseIntent=function(t,n,o){var u={interval:100,sensitivity:6,timeout:0};u="object"==typeof t?e.extend(u,t):e.isFunction(n)?e.extend(u,{over:t,out:n,selector:o}):e.extend(u,{over:t,out:t,selector:n});var s,m,i,r,I=function(e){s=e.pageX,m=e.pageY},a=function(t,n){return n.mouseIntent_t=clearTimeout(n.mouseIntent_t),Math.sqrt((i-s)*(i-s)+(r-m)*(r-m))<u.sensitivity?(e(n).off("mousemove.mouseIntent",I),n.mouseIntent_s=!0,u.over.apply(n,[t])):(i=s,r=m,n.mouseIntent_t=setTimeout(function(){a(t,n)},u.interval),void 0)},c=function(e,t){return t.mouseIntent_t=clearTimeout(t.mouseIntent_t),t.mouseIntent_s=!1,u.out.apply(t,[e])},v=function(t){var n=e.extend({},t),o=this;o.mouseIntent_t&&(o.mouseIntent_t=clearTimeout(o.mouseIntent_t)),"mouseenter"===t.type?(i=n.pageX,r=n.pageY,e(o).on("mousemove.mouseIntent",I),o.mouseIntent_s||(o.mouseIntent_t=setTimeout(function(){a(n,o)},u.interval))):(e(o).off("mousemove.mouseIntent",I),o.mouseIntent_s&&(o.mouseIntent_t=setTimeout(function(){c(n,o)},u.timeout)))};return this.on({"mouseenter.mouseIntent":v,"mouseleave.mouseIntent":v},u.selector)}}(jQuery);
                //-----

                $("#uetemplatesLyt").mouseIntent(f_slideOut);
                function f_slideOut(){
                    $("#uesimulatorLyt").addClass("move");
                }
                $("#uetemplateLyt").mouseIntent(f_slideIn);
                function f_slideIn(){
                    $("#uesimulatorLyt").removeClass("move");
                }

            }
        }
    })