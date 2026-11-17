#ifndef FEES_H
#define FEES_H

void updateFee();                 // Set or change total course fee
void addFeeInstallment();         // Record one additional payment
void viewAllFees();
void viewStudentFee(int studentId);
void viewFeeInstallments();

#endif
