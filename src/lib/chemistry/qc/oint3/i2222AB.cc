#include <chemistry/qc/oint3/build.h>
int sc::BuildIntV3::i2222eAB(){
/* the cost is 285 */
double t1;
double t2;
double t3;
double t4;
double t5;
double t6;
double t7;
double t8;
double t9;
double t10;
double t11;
double t12;
double t13;
double t14;
double t15;
double t16;
double t17;
double t18;
double t19;
double t20;
double t21;
double t22;
double t23;
double t24;
double t25;
double t26;
double t27;
double t28;
double t29;
double t30;
double t31;
double t32;
double t33;
double t34;
double t35;
double t36;
double t37;
double t38;
double t39;
double t40;
double t41;
double t42;
double t43;
double t44;
double t45;
double t46;
double t47;
double t48;
double t49;
double t50;
double t51;
double t52;
double t53;
double t54;
double t55;
t1=0.5*int_v_ooze;
double***RESTRICT int_v_list0=int_v_list(0);
double**RESTRICT int_v_list00=int_v_list0[0];
double*RESTRICT int_v_list002=int_v_list00[2];
t2=t1*int_v_list002[0];
t3=int_v_W0-int_v_p340;
double*RESTRICT int_v_list003=int_v_list00[3];
t4=t3*int_v_list003[0];
t5=int_v_p340-int_v_r30;
t6=t5*int_v_list002[0];
t7=t6+t4;
t4=int_v_W0-int_v_p120;
t6=t4*t7;
t8=t6+t2;
t6=int_v_ooze*2;
t9=0.5*t6;
t6=t9*t8;
t10=int_v_zeta12*int_v_ooze;
t11=int_v_oo2zeta34*t10;
t10=t11*(-1);
t11=t10*int_v_list002[0];
double*RESTRICT int_v_list001=int_v_list00[1];
t12=int_v_oo2zeta34*int_v_list001[0];
t13=t12+t11;
t11=t3*t7;
t12=t11+t13;
t11=t3*int_v_list002[0];
t14=t5*int_v_list001[0];
t15=t14+t11;
t11=t5*t15;
t14=t11+t12;
t11=int_v_zeta34*int_v_ooze;
t12=int_v_oo2zeta12*t11;
t11=(-1)*t12;
t12=t11*t14;
t14=t12+t6;
t6=t10*int_v_list001[0];
double*RESTRICT int_v_list000=int_v_list00[0];
t16=int_v_oo2zeta34*int_v_list000[0];
t17=t16+t6;
t6=t3*t15;
t16=t6+t17;
t6=t3*int_v_list001[0];
t18=t5*int_v_list000[0];
t19=t18+t6;
t6=t5*t19;
t18=t6+t16;
t6=int_v_oo2zeta12*t18;
t16=t6+t14;
t14=t9*t7;
t18=t10*int_v_list003[0];
t10=int_v_oo2zeta34*int_v_list002[0];
t20=t10+t18;
double*RESTRICT int_v_list004=int_v_list00[4];
t10=t3*int_v_list004[0];
t18=t5*int_v_list003[0];
t21=t18+t10;
t10=t3*t21;
t3=t10+t20;
t10=t5*t7;
t5=t10+t3;
t3=t4*t5;
t10=t3+t14;
t3=t4*t10;
t14=t3+t16;
double***RESTRICT int_v_list2=int_v_list(2);
double**RESTRICT int_v_list22=int_v_list2[2];
double*RESTRICT int_v_list220=int_v_list22[0];
int_v_list220[35]=t14;
t3=int_v_W2-int_v_p342;
t16=t3*int_v_list003[0];
t18=int_v_p342-int_v_r32;
t22=t18*int_v_list002[0];
t23=t22+t16;
t16=t4*t23;
t22=t1*t16;
t24=t3*t7;
t25=t18*t15;
t26=t25+t24;
t24=t11*t26;
t25=t24+t22;
t26=t3*t15;
t27=t18*t19;
t28=t27+t26;
t26=int_v_oo2zeta12*t28;
t27=t26+t25;
t25=t1*t23;
t28=t3*t21;
t29=t18*t7;
t30=t29+t28;
t28=t4*t30;
t29=t28+t25;
t28=t4*t29;
t31=t28+t27;
int_v_list220[34]=t31;
t27=int_v_W1-int_v_p341;
t28=t27*int_v_list003[0];
t32=int_v_p341-int_v_r31;
t33=t32*int_v_list002[0];
t34=t33+t28;
t28=t4*t34;
t33=t1*t28;
t35=t27*t7;
t36=t32*t15;
t37=t36+t35;
t35=t11*t37;
t36=t35+t33;
t37=t27*t15;
t15=t32*t19;
t19=t15+t37;
t15=int_v_oo2zeta12*t19;
t19=t15+t36;
t36=t1*t34;
t37=t27*t21;
t21=t32*t7;
t38=t21+t37;
t21=t4*t38;
t37=t21+t36;
t21=t4*t37;
t39=t21+t19;
int_v_list220[33]=t39;
t19=t3*t23;
t21=t13+t19;
t19=t3*int_v_list002[0];
t40=t18*int_v_list001[0];
t41=t40+t19;
t19=t18*t41;
t40=t19+t21;
t19=t11*t40;
t21=t3*t41;
t40=t17+t21;
t21=t3*int_v_list001[0];
t41=t18*int_v_list000[0];
t42=t41+t21;
t21=t18*t42;
t41=t21+t40;
t21=int_v_oo2zeta12*t41;
t40=t21+t19;
t41=t3*int_v_list004[0];
t42=t18*int_v_list003[0];
t43=t42+t41;
t41=t3*t43;
t42=t20+t41;
t41=t18*t23;
t43=t41+t42;
t41=t4*t43;
t42=t4*t41;
t44=t42+t40;
int_v_list220[32]=t44;
t42=t3*t34;
t45=t27*int_v_list002[0];
t46=t32*int_v_list001[0];
t47=t46+t45;
t45=t18*t47;
t46=t45+t42;
t42=t11*t46;
t45=t3*t47;
t46=t27*int_v_list001[0];
t48=t32*int_v_list000[0];
t49=t48+t46;
t46=t18*t49;
t48=t46+t45;
t45=int_v_oo2zeta12*t48;
t46=t45+t42;
t48=t27*int_v_list004[0];
t50=t32*int_v_list003[0];
t51=t50+t48;
t48=t3*t51;
t3=t18*t34;
t18=t3+t48;
t3=t4*t18;
t48=t4*t3;
t50=t48+t46;
int_v_list220[31]=t50;
t46=t27*t34;
t48=t13+t46;
t13=t32*t47;
t46=t13+t48;
t13=t11*t46;
t11=t27*t47;
t46=t17+t11;
t11=t32*t49;
t17=t11+t46;
t11=int_v_oo2zeta12*t17;
t17=t11+t13;
t46=t27*t51;
t27=t20+t46;
t20=t32*t34;
t32=t20+t27;
t20=t4*t32;
t27=t4*t20;
t4=t27+t17;
int_v_list220[30]=t4;
t27=int_v_W2-int_v_p122;
t46=t27*t10;
int_v_list220[29]=t46;
t47=t1*t8;
t8=t27*t29;
t48=t8+t47;
int_v_list220[28]=t48;
t8=t27*t37;
int_v_list220[27]=t8;
t49=t9*t16;
t16=t27*t41;
t51=t16+t49;
int_v_list220[26]=t51;
t16=t27*t3;
t49=t33+t16;
int_v_list220[25]=t49;
t16=t27*t20;
int_v_list220[24]=t16;
t33=int_v_W1-int_v_p121;
t52=t10*t33;
int_v_list220[23]=t52;
t10=t33*t29;
int_v_list220[22]=t10;
t29=t33*t37;
t37=t47+t29;
int_v_list220[21]=t37;
t29=t33*t41;
int_v_list220[20]=t29;
t41=t33*t3;
t3=t22+t41;
int_v_list220[19]=t3;
t22=t9*t28;
t28=t33*t20;
t20=t28+t22;
int_v_list220[18]=t20;
t22=t6+t12;
t6=t27*t5;
t12=t27*t6;
t6=t12+t22;
int_v_list220[17]=t6;
t12=t27*t7;
t28=t1*t12;
t12=t24+t28;
t28=t26+t12;
t12=t27*t30;
t41=t1*t7;
t47=t41+t12;
t12=t27*t47;
t47=t12+t28;
int_v_list220[16]=t47;
t12=t15+t35;
t28=t27*t38;
t53=t27*t28;
t28=t53+t12;
int_v_list220[15]=t28;
t12=t27*t23;
t53=t2+t12;
t12=t9*t53;
t53=t19+t12;
t12=t21+t53;
t19=t9*t23;
t21=t27*t43;
t53=t21+t19;
t19=t27*t53;
t21=t19+t12;
int_v_list220[14]=t21;
t12=t27*t34;
t19=t1*t12;
t12=t42+t19;
t19=t45+t12;
t12=t27*t18;
t53=t36+t12;
t12=t27*t53;
t36=t12+t19;
int_v_list220[13]=t36;
t12=t27*t32;
t19=t27*t12;
t12=t17+t19;
int_v_list220[12]=t12;
t17=t33*t5;
t5=t27*t17;
int_v_list220[11]=t5;
t19=t33*t30;
t30=t27*t19;
t53=t33*t7;
t7=t1*t53;
t53=t7+t30;
int_v_list220[10]=t53;
t30=t33*t38;
t38=t41+t30;
t30=t27*t38;
int_v_list220[9]=t30;
t41=t33*t23;
t23=t9*t41;
t54=t33*t43;
t43=t27*t54;
t55=t43+t23;
int_v_list220[8]=t55;
t23=t33*t34;
t43=t2+t23;
t2=t1*t43;
t23=t33*t18;
t18=t25+t23;
t23=t27*t18;
t25=t23+t2;
int_v_list220[7]=t25;
t2=t9*t34;
t23=t33*t32;
t32=t23+t2;
t2=t27*t32;
int_v_list220[6]=t2;
t23=t33*t17;
t17=t22+t23;
int_v_list220[5]=t17;
t22=t26+t24;
t23=t33*t19;
t19=t23+t22;
int_v_list220[4]=t19;
t22=t35+t7;
t7=t15+t22;
t15=t33*t38;
t22=t15+t7;
int_v_list220[3]=t22;
t7=t33*t54;
t15=t40+t7;
int_v_list220[2]=t15;
t7=t1*t41;
t1=t42+t7;
t7=t45+t1;
t1=t33*t18;
t18=t1+t7;
int_v_list220[1]=t18;
t1=t9*t43;
t7=t13+t1;
t1=t11+t7;
t7=t33*t32;
t9=t7+t1;
int_v_list220[0]=t9;
return 1;}
