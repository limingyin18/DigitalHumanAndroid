#include "Constraint.hpp"

using namespace std;
using namespace Eigen;
using namespace CranePhysics;

Stretching::Stretching(Rigidbody &r0, Rigidbody &r1, float restL, float compress, float stretch)
	: rb0{r0}, rb1{r1}, restLength{restL}, compressionStiffness{compress}, stretchStiffness{stretch}
{
}

void Stretching::solveConstraint()
{
	float wSum = rb0.invMass + rb1.invMass;
	if (wSum == 0.f)
		return;

	Vector3f n = rb0.position - rb1.position;
	float d = n.norm();
	n.normalize();

	Vector3f corr;
	if (d < restLength)
		corr = compressionStiffness * n * (d - restLength) / wSum;
	else
		corr = stretchStiffness * n * (d - restLength) / wSum;

	rb0.position -= rb0.invMass * corr;
	rb1.position += rb1.invMass * corr;
}

ShapeMatchingConstraint::ShapeMatchingConstraint(std::vector<Rigidbody *> &rs)
	: mX0(rs.size()), mX(rs.size()), mCorr(rs.size()), mW(rs.size()), mNumClusters(rs.size()),
	  rbs(rs)
{
	for (size_t i = 0; i < rbs.size(); ++i)
	{
		mX0[i] = rbs[i]->position;
		mW[i] = rbs[i]->invMass;
	}

	mInvRestMat.setIdentity();

	// center of mass
	mRestCM.setZero();
	float wsum = 0.f;
	for (size_t i = 0; i < rbs.size(); ++i)
	{
		float wi = 1.0f / mW[i];
		mRestCM += mX0[i] * wi;
		wsum += wi;
	}
	mRestCM /= wsum;

	// A
	Matrix3f A;
	A.setZero();
	for (size_t i = 0; i < rbs.size(); ++i)
	{
		const Vector3f qi = mX0[i] - mRestCM;
		float wi = 1.0f / mW[i];
		float x2 = wi * qi[0] * qi[0];
		float y2 = wi * qi[1] * qi[1];
		float z2 = wi * qi[2] * qi[2];
		float xy = wi * qi[0] * qi[1];
		float xz = wi * qi[0] * qi[2];
		float yz = wi * qi[1] * qi[2];
		A(0, 0) += x2;
		A(0, 1) += xy;
		A(0, 2) += xz;
		A(1, 0) += xy;
		A(1, 1) += y2;
		A(1, 2) += yz;
		A(2, 0) += xz;
		A(2, 1) += yz;
		A(2, 2) += z2;
	}

	mInvRestMat = A.inverse();
}

ParticleRigidbodyCollisionConstraint::ParticleRigidbodyCollisionConstraint(
	Particle &p, Rigidbody &rb, Eigen::Vector3f cN, float pt)
	: particle{p}, rigidbody{rb}, contactNormal{cN}, penetration{pt}
{
}

void ParticleRigidbodyCollisionConstraint::solveConstraint()
{
	particle.position += penetration * contactNormal;
}

void ParticleRigidbodyCollisionConstraint::solveVelocity()
{
	particle.velocity = particle.velocityPrime - 0.9f*(2 * particle.velocityPrime.dot(contactNormal)) * contactNormal;
}

ParticleCubeCollisionConstraint::ParticleCubeCollisionConstraint(
	Particle &p, Cube &rb, Eigen::Vector3f cN, float pt)
	: CollisionConstraint(p, rb, cN, pt), particle{p}, cube{rb}
{
}
void ParticleCubeCollisionConstraint::solveConstraint()
{
	auto rect = CollisionDetectFunction(particle, cube);

	if (rect.has_value())
	{
		Vector3f contactNormal = rect.value().first;
		contactNormal.normalize();
		float penetration = rect.value().second;
		particle.position += penetration * contactNormal;
	}
}

void jacobiRotate(Matrix3f &A, Matrix3f &R, int p, int q)
{
	// rotates A through phi in pq-plane to set A(p,q) = 0
	// rotation stored in R whose columns are eigenvectors of A
	if (A(p, q) == 0.0)
		return;

	float d = (A(p, p) - A(q, q)) / (static_cast<float>(2.0) * A(p, q));
	float t = static_cast<float>(1.0) / (fabs(d) + sqrt(d * d + static_cast<float>(1.0)));
	if (d < 0.0)
		t = -t;
	float c = static_cast<float>(1.0) / sqrt(t * t + 1);
	float s = t * c;
	A(p, p) += t * A(p, q);
	A(q, q) -= t * A(p, q);
	A(p, q) = A(q, p) = 0.0;
	// transform A
	int k;
	for (k = 0; k < 3; k++)
	{
		if (k != p && k != q)
		{
			float Akp = c * A(k, p) + s * A(k, q);
			float Akq = -s * A(k, p) + c * A(k, q);
			A(k, p) = A(p, k) = Akp;
			A(k, q) = A(q, k) = Akq;
		}
	}
	// store rotation in R
	for (k = 0; k < 3; k++)
	{
		float Rkp = c * R(k, p) + s * R(k, q);
		float Rkq = -s * R(k, p) + c * R(k, q);
		R(k, p) = Rkp;
		R(k, q) = Rkq;
	}
}

void eigenDecomposition(const Matrix3f &A, Matrix3f &eigenVecs, Vector3f &eigenVals)
{
	const int numJacobiIterations = 10;
	const float epsilon = static_cast<float>(1e-15);

	Matrix3f D = A;

	// only for symmetric matrices!
	eigenVecs.setIdentity(); // unit matrix
	int iter = 0;
	while (iter < numJacobiIterations)
	{ // 3 off diagonal elements
		// find off diagonal element with maximum modulus
		int p, q;
		float a, max;
		max = fabs(D(0, 1));
		p = 0;
		q = 1;
		a = fabs(D(0, 2));
		if (a > max)
		{
			p = 0;
			q = 2;
			max = a;
		}
		a = fabs(D(1, 2));
		if (a > max)
		{
			p = 1;
			q = 2;
			max = a;
		}
		// all small enough -> done
		if (max < epsilon)
			break;
		// rotate matrix with respect to that element
		jacobiRotate(D, eigenVecs, p, q);
		iter++;
	}
	eigenVals[0] = D(0, 0);
	eigenVals[1] = D(1, 1);
	eigenVals[2] = D(2, 2);
}

/** Perform polar decomposition A = (U D U^T) R
*/
void polarDecomposition(const Matrix3f &A, Matrix3f &R, Matrix3f &U, Matrix3f &D)
{
	// A = SR, where S is symmetric and R is orthonormal
	// -> S = (A A^T)^(1/2)

	// A = U D U^T R

	Matrix3f AAT;
	AAT(0, 0) = A(0, 0) * A(0, 0) + A(0, 1) * A(0, 1) + A(0, 2) * A(0, 2);
	AAT(1, 1) = A(1, 0) * A(1, 0) + A(1, 1) * A(1, 1) + A(1, 2) * A(1, 2);
	AAT(2, 2) = A(2, 0) * A(2, 0) + A(2, 1) * A(2, 1) + A(2, 2) * A(2, 2);

	AAT(0, 1) = A(0, 0) * A(1, 0) + A(0, 1) * A(1, 1) + A(0, 2) * A(1, 2);
	AAT(0, 2) = A(0, 0) * A(2, 0) + A(0, 1) * A(2, 1) + A(0, 2) * A(2, 2);
	AAT(1, 2) = A(1, 0) * A(2, 0) + A(1, 1) * A(2, 1) + A(1, 2) * A(2, 2);

	AAT(1, 0) = AAT(0, 1);
	AAT(2, 0) = AAT(0, 2);
	AAT(2, 1) = AAT(1, 2);

	R.setIdentity();
	Vector3f eigenVals;
	eigenDecomposition(AAT, U, eigenVals);

	float d0 = sqrt(eigenVals[0]);
	float d1 = sqrt(eigenVals[1]);
	float d2 = sqrt(eigenVals[2]);
	D.setZero();
	D(0, 0) = d0;
	D(1, 1) = d1;
	D(2, 2) = d2;

	const float eps = static_cast<float>(1e-15);

	float l0 = eigenVals[0];
	if (l0 <= eps)
		l0 = 0.0;
	else
		l0 = static_cast<float>(1.0) / d0;
	float l1 = eigenVals[1];
	if (l1 <= eps)
		l1 = 0.0;
	else
		l1 = static_cast<float>(1.0) / d1;
	float l2 = eigenVals[2];
	if (l2 <= eps)
		l2 = 0.0;
	else
		l2 = static_cast<float>(1.0) / d2;

	Matrix3f S1;
	S1(0, 0) = l0 * U(0, 0) * U(0, 0) + l1 * U(0, 1) * U(0, 1) + l2 * U(0, 2) * U(0, 2);
	S1(1, 1) = l0 * U(1, 0) * U(1, 0) + l1 * U(1, 1) * U(1, 1) + l2 * U(1, 2) * U(1, 2);
	S1(2, 2) = l0 * U(2, 0) * U(2, 0) + l1 * U(2, 1) * U(2, 1) + l2 * U(2, 2) * U(2, 2);

	S1(0, 1) = l0 * U(0, 0) * U(1, 0) + l1 * U(0, 1) * U(1, 1) + l2 * U(0, 2) * U(1, 2);
	S1(0, 2) = l0 * U(0, 0) * U(2, 0) + l1 * U(0, 1) * U(2, 1) + l2 * U(0, 2) * U(2, 2);
	S1(1, 2) = l0 * U(1, 0) * U(2, 0) + l1 * U(1, 1) * U(2, 1) + l2 * U(1, 2) * U(2, 2);

	S1(1, 0) = S1(0, 1);
	S1(2, 0) = S1(0, 2);
	S1(2, 1) = S1(1, 2);

	R = S1 * A;

	// stabilize
	Vector3f c0, c1, c2;
	c0 = R.col(0);
	c1 = R.col(1);
	c2 = R.col(2);

	if (c0.squaredNorm() < eps)
		c0 = c1.cross(c2);
	else if (c1.squaredNorm() < eps)
		c1 = c2.cross(c0);
	else
		c2 = c0.cross(c1);
	R.col(0) = c0;
	R.col(1) = c1;
	R.col(2) = c2;
}

void ShapeMatchingConstraint::solveConstraint()
{
	for (size_t i = 0; i < rbs.size(); ++i)
	{
		mX[i] = rbs[i]->position;
		mCorr[i].setZero();
	}

	// center of mass
	Vector3f cm(0.0, 0.0, 0.0);
	float wsum = 0.0;
	for (size_t i = 0; i < rbs.size(); i++)
	{
		float wi = 1.0f / mW[i];
		cm += mX[i] * wi;
		wsum += wi;
	}
	cm /= wsum;

	// A
	Matrix3f mat;
	mat.setZero();
	for (size_t i = 0; i < rbs.size(); i++)
	{
		Vector3f q = mX0[i] - mRestCM;
		Vector3f p = mX[i] - cm;

		float w = 1.0f / mW[i];
		p *= w;

		mat(0, 0) += p[0] * q[0];
		mat(0, 1) += p[0] * q[1];
		mat(0, 2) += p[0] * q[2];
		mat(1, 0) += p[1] * q[0];
		mat(1, 1) += p[1] * q[1];
		mat(1, 2) += p[1] * q[2];
		mat(2, 0) += p[2] * q[0];
		mat(2, 1) += p[2] * q[1];
		mat(2, 2) += p[2] * q[2];
	}

	mat = mat * mInvRestMat;

	Matrix3f R, U, D;
	R = mat;

	polarDecomposition(mat, R, U, D);

	for (size_t i = 0; i < rbs.size(); i++)
	{
		Vector3f goal = cm + R * (mX0[i] - mRestCM);
		mCorr[i] = (goal - mX[i]);
	}
}

LongRangeAttachmentConstraint::LongRangeAttachmentConstraint(
	Rigidbody &at, Rigidbody &r1, float restL, float stretch)
	: attachemnt{at}, rb1{r1}, restLength{restL}, stretchStiffness{stretch}
{
}

void LongRangeAttachmentConstraint::solveConstraint()
{
	Vector3f n = attachemnt.position - rb1.position;
	float d = n.norm();
	n.normalize();
	if (d > restLength)
		rb1.position += stretchStiffness * n * (d - restLength);
}

FollowTheLeaderConstraint::FollowTheLeaderConstraint(
	vector<shared_ptr<Rigidbody>> &rs) : rbs{rs}, ds{}
{
	ds.resize(rbs.size() - 1);
	for (size_t i = 0; i < ds.size(); ++i)
	{
		ds[i] = (rbs[i]->position - rbs[i + 1]->position).norm();
	}
}

void FollowTheLeaderConstraint::solveConstraint()
{
	for (size_t i = 0; i < rbs.size() - 1; ++i)
	{
		Vector3f n = rbs[i]->position - rbs[i+1]->position;
		float d = n.norm();
		n.normalize();

		rbs[i+1]->position += 0.5f*(n * (d - ds[i]));
	}
}

CranePhysics::CollisionConstraint::CollisionConstraint(Rigidbody& rb1, Rigidbody& rb2, Eigen::Vector3f cN, float pt) :
	rb1{rb1}, rb2{rb2}, contactNormal{cN}, penetration{pt}
{
}

void CranePhysics::CollisionConstraint::solveVelocity()
{
	rb1.velocity = rb1.velocityPrime - 0.9f*(2 * rb1.velocityPrime.dot(contactNormal)) * contactNormal;
}
