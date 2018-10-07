pipeline {

  agent none

  stages {

    stage('Builds & Tests') {
      parallel {

        stage('Basic Checks') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
            }
          }

          stages {
            stage('License Checks') {
              steps {
                sh './scripts/check-license-header.py'
              }
            }
            stage('Style check') {
              steps {
                sh './scripts/apply-style.py -w -a'
              }
            }
          }
        } // basic checks

        stage('Clang 6 Debug') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
            }
          }

          stages {
            stage('Debug Build') {
              steps {
                sh './scripts/ci-tool.py -B Debug'
              }
            }
            stage('Debug Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Debug'
              }
            }
            stage('Static Analysis') {
              steps {
                sh './scripts/run-static-analysis.py build-debug/'
              }
            }
          }
        } // clang 6 debug

        stage('Clang 6 Release') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
            }
          }

          stages {
            stage('Release Build') {
              steps {
                sh './scripts/ci-tool.py -B Release'
              }
            }
            stage('Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Release'
              }
            }
          }

          post {
            always {
              junit 'build-release/TestResults.xml'
            }
          }
        } // clang 6 release

      } // parallel
    } // build & test

  } // stages

} // pipeline

