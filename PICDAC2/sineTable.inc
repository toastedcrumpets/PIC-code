;Full lookup table for sine

;Get the index from the counter, and leave the sine val in W
sine_table
	movf f_hi,W

	addlw	LOW(sine_interp_table)
	movwf	TBLPTRL

	clrf TBLPTRH
	movlw 	HIGH(sine_interp_table)
	addwfc	TBLPTRH,F

	clrf TBLPTRU
	movlw 	UPPER(sine_interp_table)
	addwfc	TBLPTRU,F

	TBLRD*
	movf TABLAT,W

	return

sine_fulltable
 db .127,.128,.129,.129,.130,.131,.132,.132
 db .133,.134,.135,.136,.136,.137,.138,.139
 db .140,.140,.141,.142,.143,.143,.144,.145
 db .146,.147,.147,.148,.149,.150,.150,.151
 db .152,.153,.154,.154,.155,.156,.157,.157
 db .158,.159,.160,.160,.161,.162,.163,.163
 db .164,.165,.166,.166,.167,.168,.169,.169
 db .170,.171,.172,.172,.173,.174,.175,.175
 db .176,.177,.177,.178,.179,.180,.180,.181
 db .182,.182,.183,.184,.185,.185,.186,.187
 db .187,.188,.189,.189,.190,.191,.191,.192
 db .193,.193,.194,.195,.195,.196,.197,.197
 db .198,.199,.199,.200,.201,.201,.202,.203
 db .203,.204,.205,.205,.206,.206,.207,.208
 db .208,.209,.209,.210,.211,.211,.212,.212
 db .213,.214,.214,.215,.215,.216,.216,.217
 db .218,.218,.219,.219,.220,.220,.221,.221
 db .222,.222,.223,.223,.224,.224,.225,.225
 db .226,.226,.227,.227,.228,.228,.229,.229
 db .230,.230,.231,.231,.232,.232,.233,.233
 db .233,.234,.234,.235,.235,.236,.236,.236
 db .237,.237,.238,.238,.238,.239,.239,.240
 db .240,.240,.241,.241,.241,.242,.242,.242
 db .243,.243,.243,.244,.244,.244,.245,.245
 db .245,.246,.246,.246,.246,.247,.247,.247
 db .248,.248,.248,.248,.249,.249,.249,.249
 db .249,.250,.250,.250,.250,.251,.251,.251
 db .251,.251,.252,.252,.252,.252,.252,.252
 db .253,.253,.253,.253,.253,.253,.253,.253
 db .254,.254,.254,.254,.254,.254,.254,.254
 db .254,.254,.255,.255,.255,.255,.255,.255
 db .255,.255,.255,.255,.255,.255,.255,.255
 db .255,.255,.255,.255,.255,.255,.255,.255
 db .255,.255,.255,.255,.255,.255,.255,.254
 db .254,.254,.254,.254,.254,.254,.254,.254
 db .254,.253,.253,.253,.253,.253,.253,.253
 db .253,.252,.252,.252,.252,.252,.252,.251
 db .251,.251,.251,.251,.250,.250,.250,.250
 db .249,.249,.249,.249,.249,.248,.248,.248
 db .248,.247,.247,.247,.246,.246,.246,.246
 db .245,.245,.245,.244,.244,.244,.243,.243
 db .243,.242,.242,.242,.241,.241,.241,.240
 db .240,.240,.239,.239,.238,.238,.238,.237
 db .237,.236,.236,.236,.235,.235,.234,.234
 db .233,.233,.233,.232,.232,.231,.231,.230
 db .230,.229,.229,.228,.228,.227,.227,.226
 db .226,.225,.225,.224,.224,.223,.223,.222
 db .222,.221,.221,.220,.220,.219,.219,.218
 db .218,.217,.216,.216,.215,.215,.214,.214
 db .213,.212,.212,.211,.211,.210,.209,.209
 db .208,.208,.207,.206,.206,.205,.205,.204
 db .203,.203,.202,.201,.201,.200,.199,.199
 db .198,.197,.197,.196,.195,.195,.194,.193
 db .193,.192,.191,.191,.190,.189,.189,.188
 db .187,.187,.186,.185,.185,.184,.183,.182
 db .182,.181,.180,.180,.179,.178,.177,.177
 db .176,.175,.175,.174,.173,.172,.172,.171
 db .170,.169,.169,.168,.167,.166,.166,.165
 db .164,.163,.163,.162,.161,.160,.160,.159
 db .158,.157,.157,.156,.155,.154,.154,.153
 db .152,.151,.150,.150,.149,.148,.147,.147
 db .146,.145,.144,.143,.143,.142,.141,.140
 db .140,.139,.138,.137,.136,.136,.135,.134
 db .133,.132,.132,.131,.130,.129,.129,.128
 db .127,.126,.125,.125,.124,.123,.122,.122
 db .121,.120,.119,.118,.118,.117,.116,.115
 db .114,.114,.113,.112,.111,.111,.110,.109
 db .108,.107,.107,.106,.105,.104,.104,.103
 db .102,.101,.100,.100,.99,.98,.97,.97
 db .96,.95,.94,.94,.93,.92,.91,.91
 db .90,.89,.88,.88,.87,.86,.85,.85
 db .84,.83,.82,.82,.81,.80,.79,.79
 db .78,.77,.77,.76,.75,.74,.74,.73
 db .72,.72,.71,.70,.69,.69,.68,.67
 db .67,.66,.65,.65,.64,.63,.63,.62
 db .61,.61,.60,.59,.59,.58,.57,.57
 db .56,.55,.55,.54,.53,.53,.52,.51
 db .51,.50,.49,.49,.48,.48,.47,.46
 db .46,.45,.45,.44,.43,.43,.42,.42
 db .41,.40,.40,.39,.39,.38,.38,.37
 db .36,.36,.35,.35,.34,.34,.33,.33
 db .32,.32,.31,.31,.30,.30,.29,.29
 db .28,.28,.27,.27,.26,.26,.25,.25
 db .24,.24,.23,.23,.22,.22,.21,.21
 db .21,.20,.20,.19,.19,.18,.18,.18
 db .17,.17,.16,.16,.16,.15,.15,.14
 db .14,.14,.13,.13,.13,.12,.12,.12
 db .11,.11,.11,.10,.10,.10,.9,.9
 db .9,.8,.8,.8,.8,.7,.7,.7
 db .6,.6,.6,.6,.5,.5,.5,.5
 db .5,.4,.4,.4,.4,.3,.3,.3
 db .3,.3,.2,.2,.2,.2,.2,.2
 db .1,.1,.1,.1,.1,.1,.1,.1
 db .0,.0,.0,.0,.0,.0,.0,.0
 db .0,.0,.0,.0,.0,.0,.0,.0
 db .0,.0,.0,.0,.0,.0,.0,.0
 db .0,.0,.0,.0,.0,.0,.0,.0
 db .0,.0,.0,.0,.0,.0,.0,.0
 db .0,.0,.0,.0,.0,.0,.0,.0
 db .0,.1,.1,.1,.1,.1,.1,.1
 db .1,.2,.2,.2,.2,.2,.2,.3
 db .3,.3,.3,.3,.4,.4,.4,.4
 db .5,.5,.5,.5,.5,.6,.6,.6
 db .6,.7,.7,.7,.8,.8,.8,.8
 db .9,.9,.9,.10,.10,.10,.11,.11
 db .11,.12,.12,.12,.13,.13,.13,.14
 db .14,.14,.15,.15,.16,.16,.16,.17
 db .17,.18,.18,.18,.19,.19,.20,.20
 db .21,.21,.21,.22,.22,.23,.23,.24
 db .24,.25,.25,.26,.26,.27,.27,.28
 db .28,.29,.29,.30,.30,.31,.31,.32
 db .32,.33,.33,.34,.34,.35,.35,.36
 db .36,.37,.38,.38,.39,.39,.40,.40
 db .41,.42,.42,.43,.43,.44,.45,.45
 db .46,.46,.47,.48,.48,.49,.49,.50
 db .51,.51,.52,.53,.53,.54,.55,.55
 db .56,.57,.57,.58,.59,.59,.60,.61
 db .61,.62,.63,.63,.64,.65,.65,.66
 db .67,.67,.68,.69,.69,.70,.71,.72
 db .72,.73,.74,.74,.75,.76,.77,.77
 db .78,.79,.79,.80,.81,.82,.82,.83
 db .84,.85,.85,.86,.87,.88,.88,.89
 db .90,.91,.91,.92,.93,.94,.94,.95
 db .96,.97,.97,.98,.99,.100,.100,.101
 db .102,.103,.104,.104,.105,.106,.107,.107
 db .108,.109,.110,.111,.111,.112,.113,.114
 db .114,.115,.116,.117,.118,.118,.119,.120
 db .121,.122,.122,.123,.124,.125,.125,.126