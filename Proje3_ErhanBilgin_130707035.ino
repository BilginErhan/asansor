#include <avr/io.h>//avr kütüphanesi
#include <avr/interrupt.h>//kesme kütüphanesi
volatile int dipSwitch=1;//dip switch 
volatile int eskiKatSayisi=0;//eski bulunduğu kat
volatile int katSayisi=1;//yeni kat sayisi
volatile int zaman1=-1;//katlar
volatile int zaman2=0;//arasındaki mesefa
volatile int hedefkat=1;//kapı kapanırken yeni bir kat kabul etmez
volatile int kapi=6;//kapı kapanma durumu
volatile int sayac=0;//dur buton durumu
volatile int zamaniSakla;
void kat(int);//alt fonksiyon kat
void hesaplamalar();//hesaplamaların olduğu alt fonksiyon
int main(void)
{
  cli();//kestinler kapatılır
  DDRB=B111111;//4,7-13 arasındaki pinler output olarak tanımlanır
  DDRD=B10010000;
  DDRC=B100000;//analog pinler input olarak tanımlanır
  TCCR1B|=(1<<WGM12);//timer1 ctc modda 
  TCCR1B|=(1<<CS12);//clk değeri 1024 olarak ayarlanır
  TCCR1B|=(1<<CS10);
  OCR1A=23437.5;//her 1.5 saniyede bir ISR fonksiyonunu tetiklemesini ister
  TIMSK1|=(1<<OCIE1A);
  attachInterrupt(dipSwitch,Dur,CHANGE);//kesme alt fonksiyonu
  //dip switch her değiştiyinde Dur alt fonksiyonu çalışmaktadır
  Serial.begin(9600);//port ekranında göstermek için gerekn tanımlama
  kat(1);//başlangıçta asansör 1. katta başlar
  sei();
  while(1)
  {
    hesaplamalar();
  }
  return 0;
}
void kat(int katS)
{
  switch(katS)
  {//gönderilen değere göre yeni kat 7 segment te gösterilir
    case 1:
    //portd 0-7 arasındaki digital pinleri
    //portb 8-13 arasındaki digital pinleri kontrol eder
    //0 low 1 high durumu içindir
      PORTD=B00000000;
      PORTB=B000011;
      break;
    case 2:
      PORTD|=B10000000;
      PORTB=B101101;
      break;
    case 3:
      PORTD|=B10000000;
      PORTB=B100111;
      break;
    case 4:
      PORTD&=B01111111;
      PORTB=B110011;
      break;
    case 5:
      PORTD|=B10000000;
      PORTB=B110110;
      break;
  }
}
ISR(TIMER1_COMPA_vect)
{
  if (sayac==0)
  {//sayac dipSwitch i durumunu göre haraket eder
    if ((zaman1==zaman2 || zaman1==-1) && kapi==7)
    {//eğer zaman lar eşit ise ve kapi kapanmışsa yeni kata ulaşıldı uyarısı verir
      Serial.print("Asansor ");
      Serial.print(katSayisi);
      Serial.println("'inci kata ulasti");
      kat(katSayisi);//fonksiyona değer yollanır
      hedefkat=1;//hedefkat 1 e eşitlenerek yeni kat değeri alınabilir duruma gelir
      kapi=8;//kapı acılır
    }
    if (kapi==7)
    {//kapı açık değilse ve asansör haraket ediyorsa bu durum port ekranında gösterilir
      //ve 7 segmentte de hangi katta olduğu gözükür
      int y;
      if (eskiKatSayisi>katSayisi)
        y=eskiKatSayisi-zaman1;
      else
        y=eskiKatSayisi+zaman1;
        kat(y);
      Serial.print("Asansor ");
      Serial.print(y);
      Serial.println("'inci katta");
      zaman1++;
    }
    if (kapi<4)
    {
      //yarım saniyelik 2 defa yanıp sönme durumu
      kapi++;
      PORTD^=(1<<4);
      if (kapi==4)
      {//kapı 4 olunca led söner ve asansör haraket eder
        Serial.print("Asansor ");
        Serial.print(eskiKatSayisi);
        Serial.print("'inci kattan ");
        Serial.print(katSayisi);
        Serial.println("'inci kata dogru harakete basladi");
        kapi=7;//kapı tekrardan 7 olur
        OCR1A=23437.5;//zaman 1.5 saniyeye eşitlenir
      }
    }
  }
}
void Dur()
{
  //kesme fonksiyonu her değişim durumunda asansörü haraket halinde ise 
  //durdurur duruyorsa haraket haline devam ettirir
  sayac++;//sayac burda 1 olur
  if (sayac==1)
  {//asansör durur ve tekrar bir kesme olduğunda sayac
    //2 olur ve asansör hareket eder ve sayac tekrardan sıfırlanır
    Serial.println("Asansor Durduruldu");
    sayac=1;
    zamaniSakla=TCNT1;//zamanı saklar
  }
  if(sayac==2)
  {
    Serial.println("Asansor yeniden harakete gecti");
    sayac=0;
    TCNT1=zamaniSakla;//kaldığı zamandan devam eder
  }
}
void hesaplamalar()
{
  if (sayac==0)
    {//sayac dipSwitch durumunu kontrol eder
      int oku,j=1;
      for (int i=PC0;i<PC5;i++)
      {//analog inputlardaki değerleri okur
        oku=PINC&(1<<i);
        if (oku==1 || oku==2 || oku==4 || oku==8 || oku==16)
        {//eğer 1,2,4,8,16 ise butonlardan birine basılmıştır
          if (katSayisi!=eskiKatSayisi && j!=katSayisi && hedefkat==1)
          {//ve aynı kattaki buton basılmamış hedefkat 1 e eşit ise içeri girer
            eskiKatSayisi=katSayisi; //eski kat yeni kata eşitlenir
            katSayisi=j;//ve yeni kat değeri alınır
            hedefkat=-1;//hedef kat -1 olurki yeni bir kat girişi olmasın
            zaman1=1;//zaman1 1 e eşitlenerek
            zaman2 = abs(eskiKatSayisi-katSayisi);//aradaki fark alınır ve o kadar saniyede ulaşması beklenir
            kapi=0;//kapi kapanma konrol eden sayac 0 lanır
            OCR1A=7812.5;//ISR fonksiyonu artık her 0.5 saniyede tetiklenir
            TCNT1=0;//ve TCNT1 clc değeri sıfırlanır
            Serial.println("Kapi kapaniyor");//portda kapı kapanıyor uyarısı verir
          }
        }
        j++;//yeni kat sayısı
      }
    }
}

