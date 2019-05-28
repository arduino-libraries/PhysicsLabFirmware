/*
 * 0x18 TEMP Temperature reference
 * 0x19 BANDGAP Bandgap voltage
 * 0x1A SCALEDCOREVCC 1/4 scaled core supply
 * 0x1B SCALEDIOVCC 1/4 scaled I/O supply
 */


// Wait for synchronization of registers between the clock domains
static __inline__ void syncADC() __attribute__((always_inline, unused));
static void syncADC() {
  while (ADC->STATUS.bit.SYNCBUSY == 1)
    ;
}

static uint32_t analogReadMux(uint32_t muxpos)
{
  syncADC();
  ADC->INPUTCTRL.bit.MUXPOS = muxpos; // Selection for the positive ADC input

  // Control A
  /*
   * Bit 1 ENABLE: Enable
   *   0: The ADC is disabled.
   *   1: The ADC is enabled.
   * Due to synchronization, there is a delay from writing CTRLA.ENABLE until the peripheral is enabled/disabled. The
   * value written to CTRL.ENABLE will read back immediately and the Synchronization Busy bit in the Status register
   * (STATUS.SYNCBUSY) will be set. STATUS.SYNCBUSY will be cleared when the operation is complete.
   *
   * Before enabling the ADC, the asynchronous clock source must be selected and enabled, and the ADC reference must be
   * configured. The first conversion after the reference is changed must not be used.
   */
  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC

  // Start conversion
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Clear the Data Ready flag
  ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;

  // Start conversion again, since The first conversion after the reference is changed must not be used.
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Store the value
  while (ADC->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  uint32_t valueRead = ADC->RESULT.reg;

  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  syncADC();

  return valueRead;
}

float getIoVcc() {
  analogReference(AR_INTERNAL1V0);
  float result = analogReadMux(0x1B) * 4 / 1024.0f;
  analogReference(AR_DEFAULT);
  return result;
}

float getCoreVcc() {
  analogReference(AR_INTERNAL1V0);
  float result = analogReadMux(0x1A) * 4 / 1024.0f;
  analogReference(AR_DEFAULT);
  return result;
}
