install( FILES northafrica.desktop westafrica.desktop southafrica.desktop centralafrica.desktop eastafrica.desktop caribbean.desktop oceania.desktop centralamerica.desktop northamerica.desktop southamerica.desktop easteurope.desktop westeurope.desktop centraleurope.desktop northeurope.desktop southeurope.desktop southasia.desktop southeastasia.desktop eastasia.desktop centralasia.desktop middleeast.desktop  DESTINATION  ${LOCALE_INSTALL_DIR}/l10n )

set(listLang C ad ae af ag ai al am an ao ar as at au aw ax az ba bb bd be bf bg bh bi bj bm bn bo br bs bt bw by bz ca cc cd cf cg ch ci ck cl cm cn co cr cu cv cx cy cz de dj dk dm do dz ec ee eg eh er es et fi fj fk fm fo fr ga gb gd ge gf gh gi gl gm gn gp gq gr gt gu gw gy hk hn hr ht hu id ie il in iq ir is it jm jo jp ke kg kh ki km kn kp kr kw ky kz la lb lc li lk lr ls lt lu lv ly ma mc md me mg mh mk ml mm mn mo mq mr ms mt mu mv mw mx my mz na nc ne nf ng ni nl no np nr nu nz om pa pe pf pg ph pk pl pm pn pr ps pt pw py qa re ro rs ru rw sa sb sc sd se sg sh si sk sl sm sn so sr st sv sy sz tc td tg th tj tk tl tm tn to tp tr tt tv tw tz ua ug us uy uz va vc ve vg vi vn vu wf ws ye yt za zm zw )

foreach (_current_LANG ${listLang})
    install(FILES ${_current_LANG}/flag.png ${_current_LANG}/entry.desktop DESTINATION ${LOCALE_INSTALL_DIR}/l10n/${_current_LANG})
endforeach(_current_LANG ${listLang})
